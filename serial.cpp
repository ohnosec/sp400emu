#include "serial.h"
#include <cstring> // for memset
#include <iostream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <linux/serial.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define _POSIX_SOURCE 1 /* POSIX compliant source */
#endif

Serial::Serial(Board &board_, const std::string &dev_)
    : board(board_), dev(dev_)
#ifdef _WIN32
      , fd(INVALID_HANDLE_VALUE), rtsEnabled(false)
#else
      , fd(-1)
#endif
      , running(true), t([this] { run(); }) {}

Serial::~Serial() {
  running = false;
  t.join();
}

#ifdef _WIN32
namespace {
std::string windowsDevicePath(const std::string &device) {
  static const std::string prefix = R"(\\.\)";
  if (device.compare(0, prefix.size(), prefix) == 0) {
    return device;
  }
  return prefix + device;
}

void reportWindowsError(const std::string &operation, DWORD error) {
  std::cerr << operation << " failed with Windows error " << error << std::endl;
}
} // namespace

void Serial::setRts(bool enabled) {
  HANDLE handle = static_cast<HANDLE>(fd);
  if (handle == INVALID_HANDLE_VALUE || enabled == rtsEnabled) {
    return;
  }

  if (!EscapeCommFunction(handle, enabled ? SETRTS : CLRRTS)) {
    reportWindowsError("Setting RTS", GetLastError());
    return;
  }
  rtsEnabled = enabled;
}

void Serial::run() {
  const std::string device = windowsDevicePath(dev);
  HANDLE handle = CreateFileA(device.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    reportWindowsError("Opening " + device, GetLastError());
    return;
  }
  fd = handle;

  DCB state = {};
  state.DCBlength = sizeof(state);
  if (!GetCommState(handle, &state)) {
    reportWindowsError("Reading serial settings", GetLastError());
    CloseHandle(handle);
    fd = INVALID_HANDLE_VALUE;
    return;
  }

  state.BaudRate = CBR_4800;
  state.ByteSize = 8;
  state.Parity = NOPARITY;
  state.StopBits = ONESTOPBIT;
  state.fBinary = TRUE;
  state.fParity = FALSE;
  state.fOutxCtsFlow = FALSE;
  state.fOutxDsrFlow = FALSE;
  state.fDtrControl = DTR_CONTROL_DISABLE;
  state.fDsrSensitivity = FALSE;
  state.fTXContinueOnXoff = TRUE;
  state.fOutX = FALSE;
  state.fInX = FALSE;
  state.fErrorChar = FALSE;
  state.fNull = FALSE;
  state.fRtsControl = RTS_CONTROL_DISABLE;
  state.fAbortOnError = FALSE;

  if (!SetCommState(handle, &state)) {
    reportWindowsError("Configuring serial port", GetLastError());
    CloseHandle(handle);
    fd = INVALID_HANDLE_VALUE;
    return;
  }

  COMMTIMEOUTS timeouts = {};
  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = 50;
  if (!SetCommTimeouts(handle, &timeouts)) {
    reportWindowsError("Configuring serial timeouts", GetLastError());
    CloseHandle(handle);
    fd = INVALID_HANDLE_VALUE;
    return;
  }

  PurgeComm(handle, PURGE_RXCLEAR);
  char buffer[255];
  while (running.load()) {
    DWORD bytesRead = 0;
    if (!ReadFile(handle, buffer, sizeof(buffer), &bytesRead, nullptr)) {
      reportWindowsError("Reading serial port", GetLastError());
      break;
    }

    setRts(board.isBusy());
    if (bytesRead > 0) {
      board.pushData(reinterpret_cast<const uint8_t *>(buffer), bytesRead);
    }
  }

  CloseHandle(handle);
  fd = INVALID_HANDLE_VALUE;
}
#else
void Serial::setRts(bool b) {
  // std::cout<<"setting RTS to "<<b<<std::endl;
  int status;
  ioctl(fd, TIOCMGET, &status);
  if (((status & TIOCM_RTS) != 0) == b) {
    return;
  }

  status ^= TIOCM_RTS;
  ioctl(fd, TIOCMSET, &status);
}

void Serial::run() {
  static const int BAUDRATE = B4800;
  struct termios tty, oldtio;
  char buf[255];
  const char *device = dev.c_str();

  fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);

  if (fd < 0) {
    std::stringstream ss;
    ss << "Cannot open " << device << std::endl;
    throw std::runtime_error(ss.str());
  }

  tcgetattr(fd, &oldtio); /* save current port settings */

  serial_struct serial;
  ioctl(fd, TIOCGSERIAL, &serial);
  serial.flags |= ASYNC_LOW_LATENCY;
  ioctl(fd, TIOCSSERIAL, &serial);

  if (tcgetattr(fd, &tty) < 0) {
    std::stringstream ss;
    ss << "Error from tcgetattr: " << strerror(errno) << std::endl;
    throw std::runtime_error(ss.str());
  }

  cfsetospeed(&tty, (speed_t)BAUDRATE);
  cfsetispeed(&tty, (speed_t)BAUDRATE);

  tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;      /* 8-bit characters */
  tty.c_cflag &= ~PARENB;  /* no parity bit */
  tty.c_cflag &= ~CSTOPB;  /* only need 1 stop bit */
  tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

  /* setup for non-canonical mode */
  tty.c_iflag &=
      ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
  tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
  tty.c_oflag &= ~OPOST;

  /* fetch bytes as they become available */
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 1;

  tcflush(fd, TCIFLUSH);

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    std::stringstream ss;
    ss << "Error from tcsetattr: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }

  // setRts(false);

  while (running.load()) { /* loop for input */
    // std::cout<<"starting read"<<std::endl;
    int res = read(fd, buf, 255);
    setRts(board.isBusy());
    if (res > 0) {
      // std::cout<<"received"<<res<<" bytes"<<std::endl;
      board.pushData(reinterpret_cast<const uint8_t *>(buf), res);
    } else {
      // std::cout<<"no data"<<std::endl;
    }
  }
  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
}
#endif

#include "tcp_input.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
constexpr double SERIAL_BITS_PER_BYTE = 10.0;
constexpr double SERIAL_BAUD_RATE = 4800.0;
constexpr auto SOCKET_POLL_INTERVAL = std::chrono::milliseconds(50);
constexpr auto READY_POLL_INTERVAL = std::chrono::milliseconds(1);

#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;

int lastSocketError() { return WSAGetLastError(); }

void closeSocket(SocketHandle socket) {
  if (socket != INVALID_SOCKET_HANDLE) {
    closesocket(socket);
  }
}
#else
using SocketHandle = int;
constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;

int lastSocketError() { return errno; }

void closeSocket(SocketHandle socket) {
  if (socket != INVALID_SOCKET_HANDLE) {
    close(socket);
  }
}
#endif

std::runtime_error socketError(const std::string &operation) {
#ifdef _WIN32
  return std::runtime_error(operation + " failed with Windows socket error " +
                            std::to_string(lastSocketError()));
#else
  return std::runtime_error(
      operation + " failed: " + std::string(std::strerror(lastSocketError())));
#endif
}

int waitUntilReadable(SocketHandle socket) {
  fd_set readSet;
  FD_ZERO(&readSet);
  FD_SET(socket, &readSet);

  timeval timeout = {};
  timeout.tv_usec =
      static_cast<long>(std::chrono::duration_cast<std::chrono::microseconds>(
                            SOCKET_POLL_INTERVAL)
                            .count());

#ifdef _WIN32
  return select(0, &readSet, nullptr, nullptr, &timeout);
#else
  return select(socket + 1, &readSet, nullptr, nullptr, &timeout);
#endif
}
} // namespace

uint16_t parseTcpPort(const std::string &text) {
  if (text.empty()) {
    throw std::invalid_argument("TCP port must be an integer from 1 to 65535");
  }

  uint32_t value = 0;
  for (const char character : text) {
    if (character < '0' || character > '9') {
      throw std::invalid_argument(
          "TCP port must be an integer from 1 to 65535");
    }
    value = value * 10 + static_cast<uint32_t>(character - '0');
    if (value > 65535) {
      throw std::invalid_argument(
          "TCP port must be an integer from 1 to 65535");
    }
  }

  if (value == 0) {
    throw std::invalid_argument("TCP port must be an integer from 1 to 65535");
  }
  return static_cast<uint16_t>(value);
}

class TcpInput::Impl {
public:
  Impl(InputTarget &target_, uint16_t requestedPort)
      : target(target_), listenSocket(INVALID_SOCKET_HANDLE),
        clientSocket(INVALID_SOCKET_HANDLE), running(false), boundPort(0)
#ifdef _WIN32
        ,
        winsockStarted(false)
#endif
  {
#ifdef _WIN32
    WSADATA data = {};
    const int startupResult = WSAStartup(MAKEWORD(2, 2), &data);
    if (startupResult != 0) {
      throw std::runtime_error("Starting Winsock failed with error " +
                               std::to_string(startupResult));
    }
    winsockStarted = true;
#endif

    try {
      openListener(requestedPort);
      running = true;
      thread = std::thread([this] { run(); });
    } catch (...) {
      closeSocket(listenSocket);
      listenSocket = INVALID_SOCKET_HANDLE;
#ifdef _WIN32
      if (winsockStarted) {
        WSACleanup();
        winsockStarted = false;
      }
#endif
      throw;
    }
  }

  ~Impl() {
    running = false;
    if (thread.joinable()) {
      thread.join();
    }
    closeSocket(clientSocket);
    closeSocket(listenSocket);
#ifdef _WIN32
    if (winsockStarted) {
      WSACleanup();
    }
#endif
  }

  uint16_t port() const { return boundPort; }

private:
  void openListener(uint16_t requestedPort) {
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET_HANDLE) {
      throw socketError("Creating TCP socket");
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(requestedPort);

    if (bind(listenSocket, reinterpret_cast<const sockaddr *>(&address),
             sizeof(address)) != 0) {
      throw socketError("Binding TCP listener to 127.0.0.1:" +
                        std::to_string(requestedPort));
    }
    if (listen(listenSocket, 1) != 0) {
      throw socketError("Listening for TCP input");
    }

    sockaddr_in boundAddress = {};
#ifdef _WIN32
    int addressSize = sizeof(boundAddress);
#else
    socklen_t addressSize = sizeof(boundAddress);
#endif
    if (getsockname(listenSocket, reinterpret_cast<sockaddr *>(&boundAddress),
                    &addressSize) != 0) {
      throw socketError("Reading TCP listener address");
    }
    boundPort = ntohs(boundAddress.sin_port);
  }

  void run() {
    std::cout << "Listening for input on 127.0.0.1:" << boundPort
              << std::endl;

    while (running.load()) {
      while (running.load() && clientSocket == INVALID_SOCKET_HANDLE) {
        const int ready = waitUntilReadable(listenSocket);
        if (ready < 0) {
          if (running.load()) {
            std::cerr << socketError("Waiting for TCP connection").what()
                      << std::endl;
          }
          return;
        }
        if (ready == 0) {
          continue;
        }

        clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET_HANDLE) {
          if (running.load()) {
            std::cerr << socketError("Accepting TCP connection").what()
                      << std::endl;
          }
          return;
        }
      }

      if (!running.load()) {
        break;
      }

      std::cout << "TCP input connected" << std::endl;
      receiveBytes();
      closeSocket(clientSocket);
      clientSocket = INVALID_SOCKET_HANDLE;
    }
  }

  void receiveBytes() {
    const std::chrono::duration<double> frameDuration(SERIAL_BITS_PER_BYTE /
                                                      SERIAL_BAUD_RATE);
    auto nextByteTime = std::chrono::steady_clock::now();
    std::array<char, 255> buffer = {};

    while (running.load()) {
      const int ready = waitUntilReadable(clientSocket);
      if (ready < 0) {
        if (running.load()) {
          std::cerr << socketError("Waiting for TCP input").what() << std::endl;
        }
        return;
      }
      if (ready == 0) {
        continue;
      }

#ifdef _WIN32
      const int received =
          recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
#else
      const ssize_t received =
          recv(clientSocket, buffer.data(), buffer.size(), 0);
#endif
      if (received < 0) {
        if (running.load()) {
          std::cerr << socketError("Reading TCP input").what() << std::endl;
        }
        return;
      }
      if (received == 0) {
        std::cout << "TCP input disconnected" << std::endl;
        return;
      }

      const size_t byteCount = static_cast<size_t>(received);
      for (size_t index = 0; index < byteCount; ++index) {
        while (running.load() && !target.isReady()) {
          std::this_thread::sleep_for(READY_POLL_INTERVAL);
        }
        if (!running.load()) {
          return;
        }

        nextByteTime = std::max(nextByteTime, std::chrono::steady_clock::now());
        const auto byte =
            static_cast<uint8_t>(static_cast<unsigned char>(buffer[index]));
        target.pushData(&byte, 1);
        nextByteTime +=
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                frameDuration);
        std::this_thread::sleep_until(nextByteTime);
      }
    }
  }

  InputTarget &target;
  SocketHandle listenSocket;
  SocketHandle clientSocket;
  std::atomic<bool> running;
  uint16_t boundPort;
  std::thread thread;
#ifdef _WIN32
  bool winsockStarted;
#endif
};

TcpInput::TcpInput(InputTarget &target, uint16_t port)
    : impl(std::make_unique<Impl>(target, port)) {}

TcpInput::~TcpInput() = default;

uint16_t TcpInput::port() const { return impl->port(); }

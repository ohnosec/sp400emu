#include "tcp_input.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
#ifdef _WIN32
using SocketHandle = SOCKET;
constexpr SocketHandle INVALID_SOCKET_HANDLE = INVALID_SOCKET;
void closeSocket(SocketHandle socket) { closesocket(socket); }
#else
using SocketHandle = int;
constexpr SocketHandle INVALID_SOCKET_HANDLE = -1;
void closeSocket(SocketHandle socket) { close(socket); }
#endif

class FakeTarget : public InputTarget {
public:
  void pushData(const uint8_t *data, size_t size) override {
    std::lock_guard<std::mutex> lock(mutex);
    bytes.insert(bytes.end(), data, data + size);
  }

  bool isReady() override { return ready.load(); }

  std::vector<uint8_t> received() {
    std::lock_guard<std::mutex> lock(mutex);
    return bytes;
  }

  std::atomic<bool> ready{false};

private:
  std::mutex mutex;
  std::vector<uint8_t> bytes;
};

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Predicate>
void waitFor(Predicate predicate, const std::string &message) {
  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::seconds(2);
  while (!predicate()) {
    if (std::chrono::steady_clock::now() >= deadline) {
      throw std::runtime_error(message);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

SocketHandle connectTo(uint16_t port) {
  const SocketHandle client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (client == INVALID_SOCKET_HANDLE) {
    throw std::runtime_error("could not create test client socket");
  }

  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);
  if (connect(client, reinterpret_cast<const sockaddr *>(&address),
              sizeof(address)) != 0) {
    closeSocket(client);
    throw std::runtime_error("could not connect test client socket");
  }
  return client;
}

void sendAll(SocketHandle socket, const std::vector<uint8_t> &bytes) {
  size_t offset = 0;
  while (offset < bytes.size()) {
#ifdef _WIN32
    const int sent =
        send(socket, reinterpret_cast<const char *>(bytes.data() + offset),
             static_cast<int>(bytes.size() - offset), 0);
#else
    const ssize_t sent =
        send(socket, bytes.data() + offset, bytes.size() - offset, 0);
#endif
    if (sent <= 0) {
      throw std::runtime_error("could not send test bytes");
    }
    offset += static_cast<size_t>(sent);
  }
}

void testPortParsing() {
  require(parseTcpPort("1") == 1, "minimum TCP port was not accepted");
  require(parseTcpPort("4040") == 4040, "normal TCP port was not accepted");
  require(parseTcpPort("65535") == 65535, "maximum TCP port was not accepted");

  for (const std::string &invalid : {"", "0", "65536", "12x", "-1"}) {
    bool rejected = false;
    try {
      parseTcpPort(invalid);
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    require(rejected, "invalid TCP port was accepted: " + invalid);
  }
}

void testBinaryDeliveryAndReadyGating() {
  FakeTarget target;
  TcpInput input(target, 0);
  const SocketHandle client = connectTo(input.port());
  const std::vector<uint8_t> expected = {0x00, 0x12, 0x41, 0x0d, 0xff};

  sendAll(client, expected);
  std::this_thread::sleep_for(std::chrono::milliseconds(25));
  require(target.received().empty(), "bytes were delivered while target busy");

  target.ready = true;
  waitFor([&target,
           &expected] { return target.received().size() == expected.size(); },
          "timed out waiting for TCP bytes");
  require(target.received() == expected,
          "TCP input did not preserve binary bytes and ordering");
  closeSocket(client);
}

void testSequentialConnections() {
  FakeTarget target;
  target.ready = true;
  TcpInput input(target, 0);

  const std::vector<uint8_t> firstBytes = {0x12, 'A', 0x0d};
  const SocketHandle firstClient = connectTo(input.port());
  sendAll(firstClient, firstBytes);
  closeSocket(firstClient);
  waitFor([&target, &firstBytes] {
    return target.received().size() == firstBytes.size();
  }, "timed out waiting for first TCP connection");

  const std::vector<uint8_t> secondBytes = {0x12, 'I', 0x0d};
  const SocketHandle secondClient = connectTo(input.port());
  sendAll(secondClient, secondBytes);
  closeSocket(secondClient);

  std::vector<uint8_t> expected = firstBytes;
  expected.insert(expected.end(), secondBytes.begin(), secondBytes.end());
  waitFor([&target, &expected] {
    return target.received().size() == expected.size();
  }, "timed out waiting for reconnected TCP client");
  require(target.received() == expected,
          "sequential TCP connections did not preserve byte ordering");
}

void testOccupiedPortIsRejectedSynchronously() {
  FakeTarget firstTarget;
  FakeTarget secondTarget;
  TcpInput first(firstTarget, 0);
  bool rejected = false;
  try {
    TcpInput second(secondTarget, first.port());
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  require(rejected, "a second listener was allowed to use an occupied port");
}
} // namespace

int main() {
  try {
    testPortParsing();
    testBinaryDeliveryAndReadyGating();
    testSequentialConnections();
    testOccupiedPortIsRejectedSynchronously();
  } catch (const std::exception &error) {
    std::cerr << "TCP input test failed: " << error.what() << std::endl;
    return 1;
  }

  std::cout << "TCP input tests passed" << std::endl;
  return 0;
}

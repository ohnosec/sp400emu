#include "file_input.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {
constexpr double SERIAL_BITS_PER_BYTE = 10.0;
constexpr double SERIAL_BAUD_RATE = 4800.0;
} // namespace

FileInput::FileInput(InputTarget &target_, const std::string &path_)
    : target(target_), path(path_), file(path, std::ios::binary),
      running(false) {
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open input file: " + path);
  }

  running = true;
  thread = std::thread([this] { run(); });
}

FileInput::~FileInput() {
  running = false;
  if (thread.joinable()) {
    thread.join();
  }
}

void FileInput::run() {
  const std::chrono::duration<double> frameDuration(SERIAL_BITS_PER_BYTE /
                                                     SERIAL_BAUD_RATE);
  const auto readyPollInterval = std::chrono::milliseconds(1);
  auto nextByteTime = std::chrono::steady_clock::now();
  char value;

  while (running.load() && file.get(value)) {
    // Mirror the RTS state used by Serial so file playback cannot overrun the
    // emulated firmware's input handling.
    while (running.load() && !target.isReady()) {
      std::this_thread::sleep_for(readyPollInterval);
    }
    if (!running.load()) {
      break;
    }

    nextByteTime = std::max(nextByteTime, std::chrono::steady_clock::now());
    const auto byte = static_cast<uint8_t>(static_cast<unsigned char>(value));
    target.pushData(&byte, 1);

    nextByteTime += std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        frameDuration);
    std::this_thread::sleep_until(nextByteTime);
  }

  if (file.bad()) {
    std::cerr << "Error while reading input file: " << path << std::endl;
  } else if (file.eof()) {
    std::cout << "Finished input file: " << path << std::endl;
  }
}

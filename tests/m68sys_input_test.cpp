#include "m68sys.h"
#include "front_panel.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr uint16_t MODE_FLAGS_ADDRESS = 0x41;
constexpr uint8_t GRAPHICS_MODE_FLAG = 1;
constexpr size_t MAX_STARTUP_INSTRUCTIONS = 100000000;
constexpr size_t MAX_INPUT_INSTRUCTIONS = 1000000;
constexpr uint64_t BOARD_CYCLES_PER_TICK = 32000;
constexpr uint64_t INPUT_DRAIN_CYCLES = 1000000;

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void runUntilInputReady(M68sys &system, size_t limit,
                        const std::string &message) {
  for (size_t count = 0; count < limit; ++count) {
    if (system.isInputReady()) {
      return;
    }
    system.step();
  }
  throw std::runtime_error(message);
}

void runCycles(M68sys &system, uint64_t minimumCycles) {
  uint64_t cycles = 0;
  while (cycles < minimumCycles) {
    cycles += system.step();
  }
}

void sendAndWait(M68sys &system, uint8_t byte) {
  require(system.isInputReady(), "firmware was not ready before input byte");
  system.pushData(byte);
  require(!system.isInputReady(),
          "firmware stayed ready with an input interrupt pending");
  runUntilInputReady(system, MAX_INPUT_INSTRUCTIONS,
                     "firmware did not finish processing input byte");
  runCycles(system, BOARD_CYCLES_PER_TICK);
}

void runInstructions(M68sys &system, size_t count) {
  for (size_t index = 0; index < count; ++index) {
    system.step();
  }
}
} // namespace

int main() {
  try {
    M68sys system(SP400_ROM_PATH);

    require(!system.isInputReady(),
            "firmware accepted input while boot interrupts were masked");
    runUntilInputReady(system, MAX_STARTUP_INSTRUCTIONS,
                       "firmware did not become ready after boot");

    system.setButtons(static_cast<uint8_t>(
        0xFF & ~FrontPanelState::LINE_FEED_MASK));
    require((system.read(0) & (1U << 3)) == 0,
            "line feed button did not map to active-low PA3");
    require((system.read(0) & (1U << 2)) != 0,
            "line feed button unexpectedly changed PA2");
    system.setButtons(static_cast<uint8_t>(
        0xFF & ~FrontPanelState::COLOR_SELECT_MASK));
    require((system.read(0) & (1U << 2)) == 0,
            "color select button did not map to active-low PA2");
    require((system.read(0) & (1U << 3)) != 0,
            "color select button unexpectedly changed PA3");
    system.setButtons(0xFF);

    sendAndWait(system, 0x12);
    runInstructions(system, 1000);
    require((system.read(MODE_FLAGS_ADDRESS) & GRAPHICS_MODE_FLAG) != 0,
            "DC2 did not select graphics mode");

    sendAndWait(system, 'A');
    sendAndWait(system, '\r');

    // The ROM's A handler shares the DC1 reset routine. Select graphics mode
    // again before sending the rest of the initialization commands.
    sendAndWait(system, 0x12);
    sendAndWait(system, 'I');
    sendAndWait(system, '\r');
    runCycles(system, INPUT_DRAIN_CYCLES);
    require((system.read(MODE_FLAGS_ADDRESS) & GRAPHICS_MODE_FLAG) != 0,
            "initialization did not finish in graphics mode");
  } catch (const std::exception &error) {
    std::cerr << "Firmware input test failed: " << error.what() << std::endl;
    return 1;
  }

  std::cout << "Firmware input tests passed" << std::endl;
  return 0;
}

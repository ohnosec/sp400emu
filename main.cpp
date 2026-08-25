#include "file_input.h"
#include "input_source.h"
#include "plotter.h"
#include "serial.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
void printUsage(const char *program) {
  std::cout << "Usage:" << std::endl
            << "  " << program << " serial_dev" << std::endl
            << "  " << program << " --file command_file" << std::endl;
}
} // namespace

int main(int argc, char **argv) {
  const bool useFile = argc >= 2 && std::string(argv[1]) == "--file";
  if ((!useFile && argc != 2) || (useFile && argc != 3)) {
    printUsage(argv[0]);
    return 1;
  }

#ifdef _WIN32
  SDL_SetMainReady();
#endif

  try {
    Board board("sp400_6805.bin");
    Plotter plotter(board, 640, 480);
    std::unique_ptr<InputSource> input;

    if (useFile) {
      input = std::make_unique<FileInput>(board, argv[2]);
    } else {
      input = std::make_unique<Serial>(board, argv[1]);
    }

    plotter.run();
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return 1;
  }

  return 0;
}

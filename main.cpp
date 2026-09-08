#include "file_input.h"
#include "input_source.h"
#include "plotter.h"
#include "serial.h"
#include "tcp_input.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
void printUsage(const char *program) {
  std::cout << "Usage:" << std::endl
            << "  " << program << " serial_dev" << std::endl
            << "  " << program << " --file command_file" << std::endl
            << "  " << program << " --tcp port" << std::endl;
}
} // namespace

int main(int argc, char **argv) {
  const bool useFile = argc >= 2 && std::string(argv[1]) == "--file";
  const bool useTcp = argc >= 2 && std::string(argv[1]) == "--tcp";
  if ((useFile && argc != 3) || (useTcp && argc != 3) ||
      (!useFile && !useTcp && argc != 2)) {
    printUsage(argv[0]);
    return 1;
  }

#ifdef _WIN32
  SDL_SetMainReady();
#endif

  try {
    Board board("sp400_6805.bin");
    std::unique_ptr<InputSource> input;

    if (useFile) {
      input = std::make_unique<FileInput>(board, argv[2]);
    } else if (useTcp) {
      input = std::make_unique<TcpInput>(board, parseTcpPort(argv[2]));
    } else {
      input = std::make_unique<Serial>(board, argv[1]);
    }

    Plotter plotter(board, 640, 480);
    plotter.run();
  } catch (const std::exception &error) {
    std::cerr << "Error: " << error.what() << std::endl;
    return 1;
  }

  return 0;
}

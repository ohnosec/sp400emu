#ifndef SP400_FILE_INPUT_H_
#define SP400_FILE_INPUT_H_

#include "board.h"
#include "input_source.h"
#include <atomic>
#include <fstream>
#include <string>
#include <thread>

class FileInput : public InputSource {
public:
  FileInput(Board &board, const std::string &path);
  ~FileInput() override;

private:
  void run();

  Board &board;
  std::string path;
  std::ifstream file;
  std::atomic<bool> running;
  std::thread thread;
};

#endif

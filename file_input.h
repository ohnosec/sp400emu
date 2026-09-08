#ifndef SP400_FILE_INPUT_H_
#define SP400_FILE_INPUT_H_

#include "input_source.h"
#include "input_target.h"
#include <atomic>
#include <fstream>
#include <string>
#include <thread>

class FileInput : public InputSource {
public:
  FileInput(InputTarget &target, const std::string &path);
  ~FileInput() override;

private:
  void run();

  InputTarget &target;
  std::string path;
  std::ifstream file;
  std::atomic<bool> running;
  std::thread thread;
};

#endif

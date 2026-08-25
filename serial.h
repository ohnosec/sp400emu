#ifndef SP400_SERIAL_H_
#define SP400_SERIAL_H_

#include "board.h"
#include "input_source.h"
#include <atomic>
#include <thread>

class Serial : public InputSource {
public:
  Serial(Board &board, const std::string &dev);
  ~Serial() override;

private:
  void run();
  void setRts(bool b);
  Board &board;
  std::string dev;
#ifdef _WIN32
  void *fd;
  bool rtsEnabled;
#else
  int fd;
#endif
  std::atomic<bool> running;
  std::thread t;
};

#endif

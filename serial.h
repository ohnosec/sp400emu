#ifndef SP400_SERIAL_H_
#define SP400_SERIAL_H_

#include "input_source.h"
#include "input_target.h"
#include <atomic>
#include <string>
#include <thread>

class Serial : public InputSource {
public:
  Serial(InputTarget &target, const std::string &dev);
  ~Serial() override;

private:
  void run();
  void setRts(bool b);
  InputTarget &target;
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

#ifndef SP400_TCP_INPUT_H_
#define SP400_TCP_INPUT_H_

#include "input_source.h"
#include "input_target.h"
#include <cstdint>
#include <memory>
#include <string>

uint16_t parseTcpPort(const std::string &text);

class TcpInput : public InputSource {
public:
  TcpInput(InputTarget &target, uint16_t port);
  ~TcpInput() override;

  uint16_t port() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl;
};

#endif

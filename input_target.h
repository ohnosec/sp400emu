#ifndef SP400_INPUT_TARGET_H_
#define SP400_INPUT_TARGET_H_

#include <cstddef>
#include <cstdint>

class InputTarget {
public:
  virtual ~InputTarget() = default;
  virtual void pushData(const uint8_t *data, size_t size) = 0;
  virtual bool isReady() = 0;
};

#endif

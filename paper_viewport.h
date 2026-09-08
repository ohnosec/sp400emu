#ifndef SP400_PAPER_VIEWPORT_H
#define SP400_PAPER_VIEWPORT_H

#include <algorithm>
#include <cstdint>

class PaperViewport {
public:
  int32_t top() const { return viewportTop; }

  bool scrollBy(int32_t delta, int32_t paperHeight, int32_t viewportHeight,
                bool plotting) {
    if (plotting) {
      return false;
    }

    return setTop(static_cast<int64_t>(viewportTop) + delta, paperHeight,
                  viewportHeight);
  }

  void followHead(int32_t headY, int32_t headScreenY, int32_t paperHeight,
                  int32_t viewportHeight) {
    setTop(static_cast<int64_t>(headY) - headScreenY, paperHeight,
           viewportHeight);
  }

private:
  bool setTop(int64_t requestedTop, int32_t paperHeight,
              int32_t viewportHeight) {
    const int64_t maximumTop =
        std::max<int64_t>(0, static_cast<int64_t>(paperHeight) -
                                static_cast<int64_t>(viewportHeight));
    const int32_t newTop = static_cast<int32_t>(
        std::clamp<int64_t>(requestedTop, 0, maximumTop));
    if (newTop == viewportTop) {
      return false;
    }
    viewportTop = newTop;
    return true;
  }

  int32_t viewportTop = 0;
};

#endif

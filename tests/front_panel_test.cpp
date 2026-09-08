#include "front_panel.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}
} // namespace

int main() {
  try {
    FrontPanelState panel;
    require((panel.buttons() & FrontPanelState::LINE_FEED_MASK) != 0,
            "line feed did not start released");
    require((panel.buttons() & FrontPanelState::COLOR_SELECT_MASK) != 0,
            "color select did not start released");

    panel.setLineFeedPressed(true);
    require(panel.lineFeedPressed(), "line feed did not become pressed");
    require((panel.buttons() & FrontPanelState::LINE_FEED_MASK) == 0,
            "PA3 was not driven active-low");
    panel.setLineFeedPressed(false);
    require(!panel.lineFeedPressed(), "line feed did not release");

    panel.pulseColorSelect(1000);
    require(panel.colorSelectPressed(), "color select pulse did not start");
    require((panel.buttons() & FrontPanelState::COLOR_SELECT_MASK) == 0,
            "PA2 was not driven active-low");
    require(!panel.update(1099), "color select pulse ended too early");
    require(panel.update(1100), "color select pulse did not end on time");
    require(!panel.colorSelectPressed(), "color select stayed pressed");

    panel.setLineFeedPressed(true);
    panel.pulseColorSelect(2000);
    panel.pulseColorSelect(2050);
    require(!panel.update(2149), "retriggered color pulse ended too early");
    require(panel.update(2150), "retriggered color pulse did not end");
    require(panel.lineFeedPressed(), "color pulse released line feed");

    const uint32_t nearRollover = std::numeric_limits<uint32_t>::max() - 50;
    panel.pulseColorSelect(nearRollover);
    require(!panel.update(48), "color pulse ended early across tick rollover");
    require(panel.update(49), "color pulse did not end across tick rollover");

    panel.pulseColorSelect(3000);
    panel.releaseAll();
    require(!panel.lineFeedPressed(), "release-all left line feed pressed");
    require(!panel.colorSelectPressed(),
            "release-all left color select pressed");
  } catch (const std::exception &error) {
    std::cerr << "Front panel tests failed: " << error.what() << std::endl;
    return 1;
  }

  std::cout << "Front panel tests passed" << std::endl;
  return 0;
}

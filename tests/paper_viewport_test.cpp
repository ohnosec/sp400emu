#include "paper_viewport.h"
#include <iostream>
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
    PaperViewport viewport;
    require(viewport.top() == 0, "viewport did not start at the paper top");

    require(viewport.scrollBy(120, 960, 480, false),
            "idle downward scroll was ignored");
    require(viewport.top() == 120, "idle scroll used the wrong offset");

    require(viewport.scrollBy(-200, 960, 480, false),
            "idle upward scroll was ignored");
    require(viewport.top() == 0, "upward scroll was not clamped at zero");

    require(viewport.scrollBy(1000, 960, 480, false),
            "idle downward boundary scroll was ignored");
    require(viewport.top() == 480,
            "downward scroll exceeded the last complete viewport");

    require(!viewport.scrollBy(-100, 960, 480, true),
            "busy plotter accepted manual scrolling");
    require(viewport.top() == 480, "busy scroll changed the viewport");

    viewport.followHead(700, 240, 960, 480);
    require(viewport.top() == 460, "viewport did not center on the head");

    viewport.followHead(-10, 240, 960, 480);
    require(viewport.top() == 0,
            "negative head position moved above the paper top");

    viewport.followHead(2000, 240, 960, 480);
    require(viewport.top() == 480,
            "head following exceeded the allocated paper");

    PaperViewport shortPaper;
    require(!shortPaper.scrollBy(100, 240, 480, false),
            "paper shorter than the viewport was scrollable");
    require(shortPaper.top() == 0,
            "short paper viewport moved away from the top");
  } catch (const std::exception &error) {
    std::cerr << "Paper viewport tests failed: " << error.what() << std::endl;
    return 1;
  }

  std::cout << "Paper viewport tests passed" << std::endl;
  return 0;
}

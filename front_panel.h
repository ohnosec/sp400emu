#ifndef SP400_FRONT_PANEL_H
#define SP400_FRONT_PANEL_H

#include <cstdint>

class FrontPanelState {
public:
  // M68sys shifts these board bits left by two: bit 0 -> PA2 and bit 1 -> PA3.
  static constexpr uint8_t COLOR_SELECT_MASK = 1U << 0;
  static constexpr uint8_t LINE_FEED_MASK = 1U << 1;
  static constexpr uint32_t COLOR_SELECT_PULSE_MS = 100;

  void setLineFeedPressed(bool pressed) {
    setPressed(LINE_FEED_MASK, pressed);
  }

  void pulseColorSelect(uint32_t nowMs) {
    setPressed(COLOR_SELECT_MASK, true);
    colorSelectReleaseMs = nowMs + COLOR_SELECT_PULSE_MS;
  }

  bool update(uint32_t nowMs) {
    if (!colorSelectPressed() || !deadlineReached(nowMs, colorSelectReleaseMs)) {
      return false;
    }
    setPressed(COLOR_SELECT_MASK, false);
    return true;
  }

  void releaseAll() {
    setLineFeedPressed(false);
    setPressed(COLOR_SELECT_MASK, false);
    colorSelectReleaseMs = 0;
  }

  uint8_t buttons() const { return value; }
  bool lineFeedPressed() const { return isPressed(LINE_FEED_MASK); }
  bool colorSelectPressed() const { return isPressed(COLOR_SELECT_MASK); }

private:
  static bool deadlineReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
  }

  void setPressed(uint8_t mask, bool pressed) {
    if (pressed) {
      value &= static_cast<uint8_t>(~mask);
    } else {
      value |= mask;
    }
  }

  bool isPressed(uint8_t mask) const { return (value & mask) == 0; }

  uint8_t value = 0xFF;
  uint32_t colorSelectReleaseMs = 0;
};

#endif

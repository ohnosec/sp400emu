#include "paper_cursor.h"
#include "paper_cursor_assets.h"

#include <cstddef>
#include <cstring>

namespace {
constexpr int CURSOR_HOT_X = 15;
constexpr int CURSOR_HOT_Y = 18;

SDL_Cursor *createHandCursor(const std::uint8_t *rgba) {
  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(
      0, paper_cursor_assets::CURSOR_WIDTH,
      paper_cursor_assets::CURSOR_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
  if (surface == nullptr) {
    return nullptr;
  }

  if (SDL_LockSurface(surface) != 0) {
    SDL_FreeSurface(surface);
    return nullptr;
  }

  constexpr std::size_t ROW_BYTES =
      static_cast<std::size_t>(paper_cursor_assets::CURSOR_WIDTH) * 4;
  for (int y = 0; y < paper_cursor_assets::CURSOR_HEIGHT; ++y) {
    auto *destination = static_cast<std::uint8_t *>(surface->pixels) +
                        static_cast<std::size_t>(y) * surface->pitch;
    std::memcpy(destination, rgba + static_cast<std::size_t>(y) * ROW_BYTES,
                ROW_BYTES);
  }

  SDL_UnlockSurface(surface);
  SDL_Cursor *cursor =
      SDL_CreateColorCursor(surface, CURSOR_HOT_X, CURSOR_HOT_Y);
  SDL_FreeSurface(surface);
  return cursor;
}
} // namespace

PaperCursors::PaperCursors()
    : openHandCursor(createHandCursor(paper_cursor_assets::OPEN_HAND_RGBA)),
      closedHandCursor(
          createHandCursor(paper_cursor_assets::CLOSED_HAND_RGBA)),
      buttonPointerCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND)) {
  if (!available()) {
    if (openHandCursor != nullptr) {
      SDL_FreeCursor(openHandCursor);
    }
    if (closedHandCursor != nullptr) {
      SDL_FreeCursor(closedHandCursor);
    }
    openHandCursor = nullptr;
    closedHandCursor = nullptr;
  }
}

PaperCursors::~PaperCursors() {
  SDL_Cursor *activeCursor = SDL_GetCursor();
  if (activeCursor == openHandCursor || activeCursor == closedHandCursor ||
      activeCursor == buttonPointerCursor) {
    SDL_SetCursor(SDL_GetDefaultCursor());
  }
  if (openHandCursor != nullptr) {
    SDL_FreeCursor(openHandCursor);
  }
  if (closedHandCursor != nullptr) {
    SDL_FreeCursor(closedHandCursor);
  }
  if (buttonPointerCursor != nullptr) {
    SDL_FreeCursor(buttonPointerCursor);
  }
}

bool PaperCursors::available() const {
  return openHandCursor != nullptr && closedHandCursor != nullptr;
}

SDL_Cursor *PaperCursors::openHand() const { return openHandCursor; }

SDL_Cursor *PaperCursors::closedHand() const { return closedHandCursor; }

SDL_Cursor *PaperCursors::buttonPointer() const { return buttonPointerCursor; }

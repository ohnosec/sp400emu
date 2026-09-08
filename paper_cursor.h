#ifndef SP400_PAPER_CURSOR_H
#define SP400_PAPER_CURSOR_H

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

class PaperCursors {
public:
  PaperCursors();
  ~PaperCursors();

  PaperCursors(const PaperCursors &) = delete;
  PaperCursors &operator=(const PaperCursors &) = delete;

  bool available() const;
  SDL_Cursor *openHand() const;
  SDL_Cursor *closedHand() const;
  SDL_Cursor *buttonPointer() const;

private:
  SDL_Cursor *openHandCursor;
  SDL_Cursor *closedHandCursor;
  SDL_Cursor *buttonPointerCursor;
};

#endif

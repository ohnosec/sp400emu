
#include "plotter.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdbool.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>

std::ostream &operator<<(std::ostream &o, const SDL_Rect &r) {
  return o << r.x << " " << r.y << " " << r.w << " " << r.h;
}

Surface::Surface(int32_t w, int32_t h)
    : buffer(w * h, 0xFFFFFF), width(w), height(h) {}

void Surface::draw(const Point &p, const Color &c) {
  if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) {
    return;
  }

  const size_t index = static_cast<size_t>(p.y) * static_cast<size_t>(width) +
                       static_cast<size_t>(p.x);
  buffer[index] =
      ((uint32_t)c.r << 16) | (((uint32_t)c.g) << 8) | (((uint32_t)c.b) << 0);
}

void Surface::resizeHeight(int32_t newHeight) {
  if (newHeight <= height) {
    return;
  }

  buffer.resize(static_cast<size_t>(width) * static_cast<size_t>(newHeight),
                0xFFFFFF);
  height = newHeight;
}

void Surface::drawTo(SDL_Renderer *renderer, const SDL_Rect &src,
                     const SDL_Rect &dst) {
  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, dst.w, dst.h);
  uint32_t *texturePixels = nullptr;
  int texturePitch = 0;
  if (SDL_LockTexture(texture, NULL, (void **)&texturePixels, &texturePitch) ==
      0) {

    // std::cout<<"get"<<src<<"->"<<dst<<" from "<<width<<"x
    // "<<height<<std::endl;;
    //  Copy the specified portion of the surface to the texture
    for (int y = 0; y < dst.h; ++y) {
      for (int x = 0; x < dst.w; ++x) {
        texturePixels[y * texturePitch / sizeof(uint32_t) + x] =
            buffer[(src.y + y) * width + (src.x + x)];
      }
    }

    // Unlock the texture
    SDL_UnlockTexture(texture);
  } else {
    std::stringstream ss;
    ss << "Failed to lock texture: " << SDL_GetError() << std::endl;
    throw std::runtime_error(ss.str());
  }
  SDL_Rect all = {0, 0, dst.w, dst.h};
  SDL_SetRenderTarget(renderer, nullptr);
  SDL_RenderCopy(renderer, texture, &all, &dst);
  SDL_DestroyTexture(texture);
}

static const Color penColors[] = {{0x00, 0x00, 0x00},
                                  {0x00, 0x00, 0xFF},
                                  {0x00, 0x80, 0x00},
                                  {0x80, 0x00, 0x00}

};

void drawFilledCircle(SDL_Renderer *renderer, int centerX, int centerY,
                      int radius) {
  for (int y = -radius; y <= radius; ++y) {
    for (int x = -radius; x <= radius; ++x) {
      if (x * x + y * y <= radius * radius) {
        SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
      }
    }
  }
}

namespace {
constexpr int BUTTON_TEXT_SCALE = 2;

bool containsPoint(const SDL_Rect &rect, int x, int y) {
  return x >= rect.x && x < rect.x + rect.w && y >= rect.y &&
         y < rect.y + rect.h;
}

const uint8_t *glyph(char character) {
  static const uint8_t d[] = {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
  static const uint8_t e[] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
  static const uint8_t f[] = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
  static const uint8_t n[] = {0x11, 0x19, 0x19, 0x15, 0x13, 0x13, 0x11};
  static const uint8_t c[] = {0x0F, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0F};
  static const uint8_t l[] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
  static const uint8_t o[] = {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t r[] = {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
  static const uint8_t u[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
  static const uint8_t blank[] = {0, 0, 0, 0, 0, 0, 0};

  switch (character) {
  case 'C':
    return c;
  case 'D':
    return d;
  case 'E':
    return e;
  case 'F':
    return f;
  case 'L':
    return l;
  case 'N':
    return n;
  case 'O':
    return o;
  case 'R':
    return r;
  case 'U':
    return u;
  default:
    return blank;
  }
}

void drawLabel(SDL_Renderer *renderer, const SDL_Rect &button,
               const std::string &label) {
  constexpr int GLYPH_WIDTH = 5;
  constexpr int GLYPH_HEIGHT = 7;
  constexpr int GLYPH_SPACING = 1;
  const int textWidth =
      (static_cast<int>(label.size()) * (GLYPH_WIDTH + GLYPH_SPACING) -
       GLYPH_SPACING) *
      BUTTON_TEXT_SCALE;
  const int startX = button.x + (button.w - textWidth) / 2;
  const int startY =
      button.y + (button.h - GLYPH_HEIGHT * BUTTON_TEXT_SCALE) / 2;

  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
  for (size_t index = 0; index < label.size(); ++index) {
    const uint8_t *rows = glyph(label[index]);
    const int glyphX = startX +
                       static_cast<int>(index) *
                           (GLYPH_WIDTH + GLYPH_SPACING) * BUTTON_TEXT_SCALE;
    for (int y = 0; y < GLYPH_HEIGHT; ++y) {
      for (int x = 0; x < GLYPH_WIDTH; ++x) {
        if ((rows[y] & (1U << (GLYPH_WIDTH - x - 1))) == 0) {
          continue;
        }
        SDL_Rect pixel = {glyphX + x * BUTTON_TEXT_SCALE,
                          startY + y * BUTTON_TEXT_SCALE, BUTTON_TEXT_SCALE,
                          BUTTON_TEXT_SCALE};
        SDL_RenderFillRect(renderer, &pixel);
      }
    }
  }
}

void drawButton(SDL_Renderer *renderer, const SDL_Rect &button,
                const std::string &label, bool pressed) {
  const uint8_t shade = pressed ? 0x38 : 0x60;
  SDL_SetRenderDrawColor(renderer, shade, shade, shade, 0xFF);
  SDL_RenderFillRect(renderer, &button);
  SDL_SetRenderDrawColor(renderer, pressed ? 0xFF : 0xC0,
                         pressed ? 0xD0 : 0xC0,
                         pressed ? 0x40 : 0xC0, 0xFF);
  SDL_RenderDrawRect(renderer, &button);
  drawLabel(renderer, button, label);
}
} // namespace

Texture::Texture(SDL_Renderer *renderer_, int32_t w, int32_t h)
    : renderer(renderer_), width(w), height(h) {
  pointsTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET, w, h);
  if (pointsTexture == nullptr) {
    std::stringstream ss;
    ss << "Texture could not be created! SDL_Error: " << SDL_GetError()
       << std::endl;
    throw std::runtime_error(ss.str());
  }
  SDL_SetRenderTarget(renderer, pointsTexture);

  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderClear(renderer);
  std::cout << "creating page" << w << " x " << h << std::endl;
}

Texture::~Texture() { SDL_DestroyTexture(pointsTexture); }

void Texture::draw(const Point &p, const Color &c) {
  SDL_SetRenderTarget(renderer, pointsTexture);
  SDL_RenderDrawPoint(renderer, p.x, p.y);
}

void Texture::resizeHeight(int32_t h) {

  SDL_Texture *newTexture = SDL_CreateTexture(
      renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, h);
  if (pointsTexture == nullptr) {
    std::stringstream ss;
    ss << "Texture could not be created! SDL_Error: " << SDL_GetError()
       << std::endl;
    throw std::runtime_error(ss.str());
  }

  SDL_SetRenderTarget(renderer, newTexture);
  SDL_Rect fillRect = {0, 0, width, height};

  SDL_RenderCopy(renderer, pointsTexture, &fillRect, &fillRect);
  SDL_SetRenderTarget(renderer, nullptr);
  SDL_DestroyTexture(pointsTexture);

  SDL_SetRenderTarget(renderer, newTexture);
  // Fill the newly created space with white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  fillRect = {0, height, width, h - height};
  SDL_RenderFillRect(renderer, &fillRect);
  pointsTexture = newTexture;
  height = h;
}

Window::Window(int32_t w, int32_t h) {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::stringstream ss;
    ss << "SDL could not initialize! SDL_Error: " << SDL_GetError()
       << std::endl;
    throw std::runtime_error(ss.str());
  }

  // Create window
  window = SDL_CreateWindow("SP 400", SDL_WINDOWPOS_UNDEFINED,
                            SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    std::stringstream ss;
    ss << "Window could not be created! SDL_Error: " << SDL_GetError()
       << std::endl;
    throw std::runtime_error(ss.str());
  }

  // Create renderer
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == nullptr) {
    std::stringstream ss;
    ss << "Renderer could not be created! SDL_Error: " << SDL_GetError()
       << std::endl;
    throw std::runtime_error(ss.str());
  }
}

Window::~Window() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void Window::clear() {
  // Reset the render target to the default window
  SDL_SetRenderTarget(renderer, nullptr);

  SDL_SetRenderDrawColor(renderer, 0x80, 0x80, 0x80, 255);
  SDL_RenderClear(renderer);
}

Plotter::Plotter(Board &board_, int32_t w, int32_t h)
    : canvas_width(w), canvas_height(h), page_width(570), page_height(480),
      motOff(550, 240),
      pageOff((canvas_width - page_width) / 2, canvas_height / 2), head(0, 0),
      lineFeedButton{pageOff.x + page_width + 8, 24,
                     canvas_width - (pageOff.x + page_width + 16), 52},
      colorSelectButton{pageOff.x + page_width + 8, 92,
                        canvas_width - (pageOff.x + page_width + 16), 52},
      win(canvas_width, canvas_height), paper(page_width, page_height),
      frontPanel(), lineFeedKeyDown(false), lineFeedMouseDown(false),
      board(board_) {}

void Plotter::makePage() {
  paper.resizeHeight(paper.getHeight() + page_height);
}

void Plotter::ensurePaperHeight(int32_t requiredHeight) {
  while (requiredHeight > paper.getHeight()) {
    makePage();
    std::cout << "made page of size " << paper.getHeight() << std::endl;
  }
}

void Plotter::updateLineFeed() {
  frontPanel.setLineFeedPressed(lineFeedKeyDown || lineFeedMouseDown);
  board.setButtons(frontPanel.buttons());
}

void Plotter::pulseColorSelect() {
  frontPanel.pulseColorSelect(SDL_GetTicks64());
  board.setButtons(frontPanel.buttons());
}

void Plotter::releaseControls() {
  SDL_CaptureMouse(SDL_FALSE);
  lineFeedKeyDown = false;
  lineFeedMouseDown = false;
  frontPanel.releaseAll();
  board.setButtons(frontPanel.buttons());
}

void Plotter::handleEvent(const SDL_Event &event, bool &quit) {
  switch (event.type) {
  case SDL_QUIT:
    releaseControls();
    quit = true;
    break;
  case SDL_WINDOWEVENT:
    if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
      releaseControls();
    }
    break;
  case SDL_KEYDOWN:
    if (event.key.repeat != 0) {
      break;
    }
    if (event.key.keysym.sym == SDLK_f) {
      lineFeedKeyDown = true;
      updateLineFeed();
    } else if (event.key.keysym.sym == SDLK_c) {
      pulseColorSelect();
    }
    break;
  case SDL_KEYUP:
    if (event.key.keysym.sym == SDLK_f) {
      lineFeedKeyDown = false;
      updateLineFeed();
    }
    break;
  case SDL_MOUSEBUTTONDOWN:
    if (event.button.button != SDL_BUTTON_LEFT) {
      break;
    }
    if (containsPoint(lineFeedButton, event.button.x, event.button.y)) {
      lineFeedMouseDown = true;
      SDL_CaptureMouse(SDL_TRUE);
      updateLineFeed();
    } else if (containsPoint(colorSelectButton, event.button.x,
                             event.button.y)) {
      pulseColorSelect();
    }
    break;
  case SDL_MOUSEBUTTONUP:
    if (event.button.button == SDL_BUTTON_LEFT && lineFeedMouseDown) {
      lineFeedMouseDown = false;
      SDL_CaptureMouse(SDL_FALSE);
      updateLineFeed();
    }
    break;
  default:
    break;
  }
}

void Plotter::drawControls() {
  drawButton(win.renderer, lineFeedButton, "FEED",
             frontPanel.lineFeedPressed());
  drawButton(win.renderer, colorSelectButton, "COLOR",
             frontPanel.colorSelectPressed());
}

void Plotter::run() {
  bool quit = false;
  SDL_Event e;
  const Color *c = penColors;
  while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
      handleEvent(e, quit);
    }
    if (frontPanel.update(SDL_GetTicks64())) {
      board.setButtons(frontPanel.buttons());
    }
    auto states = board.getStates();
    for (auto &s : states) {
      head.x = motOff.x + s.x;
      head.y = motOff.y + s.y;
      c = penColors + s.colorIdx;
      if (s.penDown) {
        if (head.y >= 0) {
          ensurePaperHeight(head.y + 1);
        }
        paper.draw(head, *c);
      }
    }

    win.clear();
    const int32_t viewportTop = std::max(0, head.y - pageOff.y);
    ensurePaperHeight(viewportTop + canvas_height);
    SDL_Rect srcRect = {0, viewportTop, page_width,
                        canvas_height}; // Full texture area
    SDL_Rect destRect = {pageOff.x, 0, page_width,
                         canvas_height}; // Half window area
    paper.drawTo(win.renderer, srcRect, destRect);
    SDL_SetRenderDrawColor(win.renderer, c->r, c->g, c->b, 255);
    drawFilledCircle(win.renderer, head.x + pageOff.x, head.y - viewportTop, 6);
    drawControls();
    SDL_RenderPresent(win.renderer);
  }
  releaseControls();
}

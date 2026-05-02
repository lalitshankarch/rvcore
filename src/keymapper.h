#include "types.h"
#include <SDL3/SDL_keycode.h>
#include <cctype>

u16 convertKey(u32 key) {
  switch (key) {
  case SDLK_RETURN:
    return 0x01;
  case SDLK_ESCAPE:
    return 0x02;
  case SDLK_LEFT:
    return 0x03;
  case SDLK_RIGHT:
    return 0x04;
  case SDLK_UP:
    return 0x05;
  case SDLK_DOWN:
    return 0x06;
  case SDLK_LCTRL:
  case SDLK_RCTRL:
    return 0x07;
  case SDLK_SPACE:
    return 0x08;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:
    return 0x09;
  case SDLK_LALT:
  case SDLK_RALT:
    return 0x0a;
  case SDLK_F2:
    return 0x0b;
  case SDLK_F3:
    return 0x0c;
  case SDLK_F4:
    return 0x0d;
  case SDLK_F5:
    return 0x0e;
  case SDLK_F6:
    return 0x0f;
  case SDLK_F7:
    return 0x10;
  case SDLK_F8:
    return 0x11;
  case SDLK_F9:
    return 0x12;
  case SDLK_F10:
    return 0x13;
  case SDLK_F11:
    return 0x14;
  case SDLK_EQUALS:
  case SDLK_PLUS:
    return 0x15;
  case SDLK_MINUS:
    return 0x16;
  default:
    return u16(std::tolower(int(key)));
  }
}
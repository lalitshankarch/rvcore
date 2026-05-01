#include "constants.h"
#include "cpu.h"
#include "debug.h"
#include "elf.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <exception>
#include <vector>

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("Usage: %s <bin>\n", argv[0]);
    return 1;
  }

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;

  try {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      EXCEPTION("Failed to initialize SDL");
    }

    window = SDL_CreateWindow("rvdoom", WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_INPUT_FOCUS);
    if (!window) {
      SDL_Quit();
      EXCEPTION("Failed to create window");
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
      SDL_DestroyWindow(window);
      SDL_Quit();
      EXCEPTION("Failed to create renderer");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH,
                                WINDOW_HEIGHT);

    std::vector<u8> memory;
    ExecSeg exec_seg = ExecSeg::read_into(memory, argv[1]);

    memory.resize(MEM_SIZE);

    Cpu cpu(memory, exec_seg.entry, exec_seg.nmembytes);

    while (true) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
          return 0;
      }

      while (true) {
        cpu.step();
        if (cpu.should_render)
          break;
      }

      void *pixels = reinterpret_cast<void *>(&memory[VRAM_START]);
      SDL_UpdateTexture(texture, NULL, pixels, WINDOW_WIDTH * sizeof(uint32_t));
      SDL_RenderClear(renderer);
      SDL_RenderTexture(renderer, texture, NULL, NULL);
      SDL_RenderPresent(renderer);

      cpu.should_render = false;
    }

  } catch (std::exception &ex) {
    std::cerr << std::endl << RED << ex.what() << RESET << std::endl;
  }

  return 0;
}
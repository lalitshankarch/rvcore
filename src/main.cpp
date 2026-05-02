#include "constants.h"
#include "cpu.h"
#include "debug.h"
#include "elf.h"
#include "keymapper.h"
#include <SDL3/SDL.h>
#include <exception>
#include <vector>

int main(int argc, const char **argv) {
  if (argc < 2) {
    printf("Usage: %s <bin>\n", argv[0]);
    return 1;
  }

  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  SDL_Texture *texture = nullptr;

  try {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      EXCEPTION("Failed to initialize SDL");
    }

    if (!SDL_CreateWindowAndRenderer("rvdoom", WINDOW_WIDTH, WINDOW_HEIGHT,
                                     SDL_WINDOW_INPUT_FOCUS, &window,
                                     &renderer)) {
      SDL_Quit();
      EXCEPTION("Failed to create window and renderer");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_XRGB8888,
                                SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH,
                                WINDOW_HEIGHT);

    std::vector<u8> memory;
    ExecSeg exec_seg = ExecSeg::read_into(memory, argv[1]);

    memory.resize(MEM_SIZE);

    Cpu cpu(memory, exec_seg.entry, exec_seg.nmembytes);

    while (true) {
      u16 *queue = reinterpret_cast<u16 *>(
          reinterpret_cast<void *>(&memory[QUEUE_START]));
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
          return 0;
        if (event.type == SDL_EVENT_KEY_DOWN ||
            event.type == SDL_EVENT_KEY_UP) {
          u16 key = convertKey(event.key.key);
          u16 keyData =
              u16((((event.type == SDL_EVENT_KEY_DOWN) ? 1 : 0) << 8) | key);
          queue[memory[QUEUE_WRITE_IDX]] = keyData;
          memory[QUEUE_WRITE_IDX]++;
          memory[QUEUE_WRITE_IDX] %= 16;
        }
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

  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
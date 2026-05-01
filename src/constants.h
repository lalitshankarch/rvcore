#pragma once

#include "types.h"

const u32 BASE_MEM_SIZE = 1024 * 1024 * 8;
const u32 STACK_START = BASE_MEM_SIZE;
const u32 VRAM_START = BASE_MEM_SIZE;
const u32 VRAM_SIZE = 640 * 400 * 4;
const u32 MEM_SIZE = BASE_MEM_SIZE + VRAM_SIZE;
const u32 WINDOW_WIDTH = 640;
const u32 WINDOW_HEIGHT = 400;
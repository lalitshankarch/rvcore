#pragma once

#include "types.h"
#include <cstdio>
#include <cstdlib>

constexpr const char *WHITE = "\033[37m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *RED = "\033[31m";
constexpr const char *RESET = "\033[0m";

#define ABORT(MSG)                                                             \
  do {                                                                         \
    ERROR_PRINT(MSG);                                                          \
    std::abort();                                                              \
  } while (0)

#define ERROR_PRINT(MSG) std::fprintf(stderr, "%s%s%s\n", RED, MSG, RESET)

#ifndef NDEBUG
#define DEBUG_PRINT(MSG) std::fprintf(stderr, "%s%s%s\n", WHITE, MSG, RESET)
#define WARN_PRINT(MSG) std::fprintf(stderr, "%s%s%s\n", YELLOW, MSG, RESET)
#else
#define DEBUG_PRINT(MSG) ((void)0)
#define WARN_PRINT(MSG) ((void)0)
#endif
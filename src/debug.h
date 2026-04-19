#pragma once

#include "types.h"
#include <exception>
#include <format>
#include <iostream>

constexpr const char *WHITE = "\033[37m";
constexpr const char *YELLOW = "\033[33m";
constexpr const char *RED = "\033[31m";
constexpr const char *RESET = "\033[0m";

#define EXCEPTION(...) throw std::runtime_error(std::format(__VA_ARGS__))

enum LogLevel : u32 {
  DEBUG,
  WARN,
  ERROR,
};

#ifndef NDEBUG
#define DEBUG_PRINT(...)                                                       \
  std::cerr << WHITE << std::format(__VA_ARGS__) << RESET << std::endl
#define WARN_PRINT(...)                                                        \
  std::cerr << YELLOW << std::format(__VA_ARGS__) << RESET << std::endl
#define ERROR_PRINT(...)                                                       \
  std::cerr << RED << std::format(__VA_ARGS__) << RESET << std::endl
#else
#define DEBUG_PRINT(...) ((void)0)
#define WARN_PRINT(...) ((void)0)
#define ERROR_PRINT(...) ((void)0)
#endif
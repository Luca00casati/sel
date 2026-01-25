#pragma once
#include <cstddef>
#include <cstdint>
#include <iostream>

namespace sel {

// using word = std::uintptr_t;
using word = std::int64_t;
using word_size = std::size_t;

template <typename... Args> void print(std::ostream &os, Args &&...args) {
  (os << ... << args);
}

template <typename... Args> void print(Args &&...args) {
  print(std::cout, std::forward<Args>(args)...);
}

template <typename... Args> void printn(std::ostream &os, Args &&...args) {
  print(os, std::forward<Args>(args)...);
  os << '\n';
}

template <typename... Args> void printn(Args &&...args) {
  printn(std::cout, std::forward<Args>(args)...);
}

[[noreturn]] void fatal(const char *msg) noexcept {
  std::fprintf(stderr, "Fatal Error: %s\n", msg);
  std::exit(EXIT_FAILURE);
}

} // namespace sel

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>
#include "sel.hpp"

class Arena {
public:
  explicit Arena(std::size_t chunk_size = kDefaultChunkSize)
      : chunk_size_(chunk_size) {
    if (chunk_size_ == 0)
      sel::fatal("arena chunk size cannot be zero");
    add_chunk(chunk_size_);
  }

  ~Arena() {
    Chunk *chunk = head_;
    while (chunk) {
      Chunk *next = chunk->next;
      std::free(chunk);
      chunk = next;
    }
  }

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;

  Arena(Arena &&other) noexcept
      : head_(other.head_), current_(other.current_),
        current_offset_(other.current_offset_), chunk_size_(other.chunk_size_) {
    other.head_ = nullptr;
    other.current_ = nullptr;
    other.current_offset_ = 0;
    other.chunk_size_ = 0;
  }

  Arena &operator=(Arena &&other) noexcept {
    if (this != &other) {
      Chunk *chunk = head_;
      while (chunk) {
        Chunk *next = chunk->next;
        std::free(chunk);
        chunk = next;
      }
      head_ = other.head_;
      current_ = other.current_;
      current_offset_ = other.current_offset_;
      chunk_size_ = other.chunk_size_;

      other.head_ = nullptr;
      other.current_ = nullptr;
      other.current_offset_ = 0;
      other.chunk_size_ = 0;
    }
    return *this;
  }

  void reset() noexcept {
    current_ = head_;
    current_offset_ = 0;
  }

  template <typename T> T *alloc(std::size_t count = 1) noexcept {
    static_assert(!std::is_void_v<T>, "arena cannot allocate void");
    if (count > SIZE_MAX / sizeof(T))
      sel::fatal("arena allocation size overflow");
    return static_cast<T *>(alloc_aligned(sizeof(T) * count, alignof(T)));
  }

  void *alloc_bytes(std::size_t size) noexcept {
    return alloc_aligned(size, alignof(std::max_align_t));
  }

  char *strdup(const char *s) noexcept {
    if (!s)
      return nullptr;
    std::size_t len = std::strlen(s) + 1;
    char *dst = alloc<char>(len);
    std::memcpy(dst, s, len);
    return dst;
  }

private:
  static constexpr std::size_t kDefaultChunkSize = 4096;

  struct Chunk {
    Chunk *next;
    std::byte data[1];
  };

  Chunk *head_ = nullptr;
  Chunk *current_ = nullptr;
  std::size_t current_offset_ = 0;
  std::size_t chunk_size_;

  std::size_t align_up(std::size_t value, std::size_t align) noexcept {
    return (value + (align - 1)) & ~(align - 1);
  }

  void add_chunk(std::size_t min_size) noexcept {
    std::size_t size = (min_size > chunk_size_) ? min_size : chunk_size_;
    // Allocate extra space for the Chunk struct itself
    std::size_t total_size = sizeof(Chunk) - 1 + size;
    Chunk *chunk = static_cast<Chunk *>(std::malloc(total_size));
    if (!chunk)
      sel::fatal("arena malloc failed for new chunk");
    chunk->next = nullptr;

    if (!head_) {
      head_ = chunk;
    } else {
      current_->next = chunk;
    }
    current_ = chunk;
    current_offset_ = 0;
  }

  void *alloc_aligned(std::size_t size, std::size_t align) noexcept {
    if ((align & (align - 1)) != 0)
      sel::fatal("arena alignment must be power of two");

    std::size_t aligned_offset = align_up(current_offset_, align);
    if (aligned_offset + size > chunk_size_) {
      // Need a new chunk
      std::size_t new_chunk_size = (size > chunk_size_) ? size : chunk_size_;
      add_chunk(new_chunk_size);
      aligned_offset = 0;
    }

    void *ptr = current_->data + aligned_offset;
    current_offset_ = aligned_offset + size;
    return ptr;
  }
};

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <new>

class Arena {
public:
    explicit Arena(std::size_t initial_size = kInitSize)
        : data_(nullptr), size_(0), offset_(0)
    {
        if (initial_size)
            grow(initial_size);
    }

    ~Arena() {
        std::free(data_);
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other) noexcept
        : data_(other.data_), size_(other.size_), offset_(other.offset_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
        other.offset_ = 0;
    }

    Arena& operator=(Arena&& other) noexcept {
        if (this != &other) {
            std::free(data_);
            data_ = other.data_;
            size_ = other.size_;
            offset_ = other.offset_;

            other.data_ = nullptr;
            other.size_ = 0;
            other.offset_ = 0;
        }
        return *this;
    }

    void reset() noexcept {
        offset_ = 0;
    }

    template <typename T>
    T* alloc(std::size_t count = 1) {
        static_assert(!std::is_void_v<T>, "cannot allocate void");
        return static_cast<T*>(
            alloc_aligned(sizeof(T) * count, alignof(T))
        );
    }

    void* alloc_bytes(std::size_t size) {
        return alloc_aligned(size, alignof(std::max_align_t));
    }

    char* strdup(const char* s) {
    std::size_t len = std::strlen(s) + 1;
    char* dst = alloc<char>(len);
    std::memcpy(dst, s, len);
    return dst;
    }

private:
    static constexpr std::size_t kInitSize = 4096;
    static constexpr std::size_t kGrowFactor = 2;

    static std::size_t align_up(std::size_t value, std::size_t align) {
        return (value + (align - 1)) & ~(align - 1);
    }

    void grow(std::size_t min_size) {
        std::size_t new_size = size_ ? size_ : kInitSize;

        while (new_size < min_size) {
            if (new_size > SIZE_MAX / kGrowFactor)
                throw std::bad_alloc();
            new_size *= kGrowFactor;
        }

        void* new_data = std::realloc(data_, new_size);
        if (!new_data)
            throw std::bad_alloc();

        data_ = static_cast<std::byte*>(new_data);
        size_ = new_size;
    }

    void* alloc_aligned(std::size_t size, std::size_t align) {
        if ((align & (align - 1)) != 0)
            throw std::invalid_argument("alignment must be power of two");

        std::size_t aligned_offset = align_up(offset_, align);

        if (size > SIZE_MAX - aligned_offset)
            throw std::bad_alloc();

        std::size_t end = aligned_offset + size;

        if (end > size_)
            grow(end);

        void* ptr = data_ + aligned_offset;
        offset_ = end;
        return ptr;
    }

    std::byte* data_;
    std::size_t size_;
    std::size_t offset_;
};

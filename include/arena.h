#include "common.h"
#pragma once

#define ARENA_GROW 2
#define ARENA_INIT_SIZE 256

typedef struct{
  byte* data;
  word_size offset;
  word_size size;
}Arena;

word_size align_up(word_size ptr, word_size allign);
void arena_grow(Arena *arena, word_size min_size);
void* arena_alloc_aligned(Arena *arena, word_size size, word_size align);
void *arena_alloc(Arena *arena, word_size size);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);

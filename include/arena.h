#include "common.h"
#pragma once

#define ARENA_GROW 2
#define ARENA_INIT_SIZE 256

typedef struct{
  byte* data;
  word_size offset;
  word_size size;
}Arena;

#define arena_alloc(arena, type) ((type *)arena_alloc_aligned((arena), sizeof(type), _Alignof(type)))
#define arena_alloc_array(arena, type, count) ((type *)arena_alloc_aligned((arena), sizeof(type) * (count), _Alignof(type)))

word_size align_up(word_size ptr, word_size align);
void arena_grow(Arena *arena, word_size min_size);
void* arena_alloc_aligned(Arena *arena, word_size size, word_size align);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);

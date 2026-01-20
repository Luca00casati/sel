#include <stdio.h>
#include <stdlib.h>
#include "arena.h"

word_size align_up(word_size ptr, word_size align) {
    return (ptr + (align - 1)) & ~(align - 1);
}

void arena_grow(Arena *arena, word_size min_size) {
    word_size new_size = arena->size ? arena->size : ARENA_INIT_SIZE;

    while (new_size < min_size)
        new_size *= ARENA_GROW;

    void *tmp = realloc(arena->data, new_size);
    if (!tmp) {
        perror("arena realloc");
        exit(1);
    }

    arena->data = tmp;
    arena->size = new_size;
}

void *arena_alloc_aligned(Arena *arena, word_size size, word_size align) {
    if ((align & (align - 1)) != 0) {
        fprintf(stderr, "alignment must be power of two\n");
        exit(1);
    }

    word_size aligned_offset = align_up(arena->offset, align);
    word_size end = aligned_offset + size;

    if (end > arena->size) {
        arena_grow(arena, end);
    }

    void *ptr = arena->data + aligned_offset;
    arena->offset = end;

    return ptr;
}


void arena_reset(Arena *arena) {
    arena->offset = 0;
}

void arena_free(Arena *arena) {
    free(arena->data);
    arena->data = NULL;
    arena->size = 0;
    arena->offset = 0;
}

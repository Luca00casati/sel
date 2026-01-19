#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define HALT_IF(cond, error) do{if(cond){perror(error);abort();}}while(0)

#ifndef NDEBUG
#define DEBUG_HALT_IF(cond, error) HALT_IF(cond, error)
#else
#define DEBUG_HALT_IF(cond, error) ((void)0)
#endif

typedef uint8_t byte;
typedef int64_t word;
typedef size_t word_size;

typedef struct{
  byte* data;
  size_t len;
}String;

#define STACK_GROW 2
#define STACK_INIT_SIZE 256

typedef struct{
  word* data;
  word_size size;
  word_size current_size;
  word_size ip;
}Stack;

typedef enum{
  OP_PLUS,
  OP_PUSH,
  OP_POP,
  OP_DROP
}Operators;

void string(String *str, const char* cstr){
  DEBUG_HALT_IF(str->data != NULL || str->len != 0, "error invalid string string\n");
  str->len = strlen(cstr);
  str->data = (byte*)malloc(sizeof(byte) * str->len);
  if (str->data == NULL){
    fprintf(stderr, "error malloc fail string\n");
    exit(1);
  }
  memcpy(str->data, cstr, str->len); 
}

void free_string(String *str){
  free(str->data);
  str->len = 0;
}

#define ARENA_GROW 2
#define ARENA_INIT_SIZE 256

typedef struct{
  byte* data;
  word_size offset;
  word_size size;
}Arena;

#define arena_alloc(arena, type) ((type *)arena_alloc_aligned((arena), sizeof(type), _Alignof(type)))
#define arena_alloc_array(arena, type, count) ((type *)arena_alloc_aligned((arena), sizeof(type) * (count), _Alignof(type)))

static inline size_t
align_up(size_t ptr, size_t align) {
    return (ptr + (align - 1)) & ~(align - 1);
}

static void
arena_grow(Arena *arena, size_t min_size) {
    size_t new_size = arena->size ? arena->size : ARENA_INIT_SIZE;

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

void *
arena_alloc_aligned(Arena *arena, size_t size, size_t align) {
    if ((align & (align - 1)) != 0) {
        fprintf(stderr, "alignment must be power of two\n");
        exit(1);
    }

    size_t aligned_offset = align_up(arena->offset, align);
    size_t end = aligned_offset + size;

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

int main(){
  Arena strings = {0};
  String exp = {0};
  string(&exp, "(+ 1 2)");
  free_string(&exp); 
}

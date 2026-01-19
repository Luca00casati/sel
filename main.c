#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define HALT_IF(cond, error) do{if(cond){fputs(error, stderr);abort();}}while(0)

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



typedef struct{
  void* data;
  word_size size;
  word_size current_size;
}Arena;

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

void arena_add(Arena *arena, word_size offset, word_size size_arr){
  //DEBUG_HALT_IF(arena->data != NULL || arena->size != 0 || arena->current_size != 0 || offset == 0 || size_arr == 0, " 
  

int main(){
  Arena strings = {0};
  String exp = {0};
  string(&exp, "(+ 1 2)");
  
}

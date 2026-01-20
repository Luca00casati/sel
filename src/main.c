#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mystring.h"
#include "arena.h"

typedef struct{
  word value;
  word_size operation_type;
  word_size type;
}Type;

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

int main(){
  //String exp = {0};
  Arena arena = {0};
  String str = arena_strdup(&arena, "hello");
  printf("str: %s, len: %ld\n", str.data, str.len);
  arena_free(&arena); 
}

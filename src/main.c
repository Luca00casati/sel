#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mystring.h"
#include "arena.h"

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
  //Arena strings = {0};
  String exp = {0};
  string(&exp, "(+ 1 2)");
  free_string(&exp); 
}

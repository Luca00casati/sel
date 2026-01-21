#include <iostream>
#include <cstddef>
#include <cstdint>
#include "arena.hpp"

using word = std::uintptr_t;
using word_size = std::size_t;

using namespace std;

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
  Arena arena;
  string_view str = arena.strdup("hello");
  cout << str << endl;
}

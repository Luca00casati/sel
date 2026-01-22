#include "arena.hpp"
#include "sel.hpp"
#include <iostream>

using sv = std::string_view;

using namespace std;
using namespace sel;

typedef struct {
  word value;
  word_size operation_type;
  word_size type;
} Type;

#define STACK_GROW 2
#define STACK_INIT_SIZE 256

typedef struct {
  Type *data;
  word_size size;
  word_size current_size;
  word_size ip;
} Stack;

typedef enum { OP_PLUS, OP_PUSH, OP_POP, OP_DROP } Operators;

int main() {
  Arena arena;
  sv str = arena.strdup("hello");
  print(str, " ,word\n");
}

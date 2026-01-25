#include "arena.hpp"
#include "sel.hpp"
#include <iostream>

using sv = std::string_view;

using namespace std;
using namespace sel;

typedef struct {
  word value;
  word_size operation_type;
  word_size value_type;
} Sel_type;

typedef struct {
  Sel_type *data;
  word_size size;
  word_size ip;
} Sel_stack;

typedef enum { OP_PLUS_INT, OP_PUSH_INT, OP_POP, OP_DROP } Sel_ops;
typedef enum { INT } Sel_types;

void emit(Sel_stack *st, Sel_ops op, int value) {
  Sel_type *i = &st->data[st->size++];
  i->operation_type = op;
  i->value = value;
  i->value_type = 0; // int
}

#include <cctype>
#include <cstdlib>

void skip_ws(const char **p) {
  while (**p && std::isspace(**p)) ++(*p);
}

int parse_int(const char **p) {
  int v = 0;
  while (std::isdigit(**p)) {
    v = v * 10 + (**p - '0');
    ++(*p);
  }
  return v;
}

void compile_expr(const char **p, Sel_stack *st);

void compile_list(const char **p, Sel_stack *st) {
  skip_ws(p);

  // expect '+'
  if (**p != '+') {
    printn("only + supported\n");
    std::exit(1);
  }
  ++(*p); // consume '+'

  skip_ws(p);

  // first expression
  compile_expr(p, st);

  // remaining expressions
  while (1) {
    skip_ws(p);

    if (**p == ')') {
      ++(*p); // consume ')'
      break;
    }

    compile_expr(p, st);
    emit(st, OP_PLUS_INT, 0);
  }
}

void compile_expr(const char **p, Sel_stack *st) {
  skip_ws(p);

  if (**p == '(') {
    ++(*p);               // consume '('
    compile_list(p, st);
    return;
  }

  if (std::isdigit(**p)) {
    int v = parse_int(p);
    emit(st, OP_PUSH_INT, v);
    return;
  }

  printn("syntax error\n");
  std::exit(1);
}

int run(Sel_stack *st) {
  int stack[64];
  int sp = 0;

  for (st->ip = 0; st->ip < st->size; ++st->ip) {
    Sel_type *i = &st->data[st->ip];

    switch (i->operation_type) {
      case OP_PUSH_INT:
        stack[sp++] = i->value;
        break;

      case OP_PLUS_INT: {
        int b = stack[--sp];
        int a = stack[--sp];
        stack[sp++] = a + b;
        break;
      }
    }
  }

  return stack[0];
}

int main() {
  Arena arena;

  const char *exp = "(+ 1  3 (+ 1 5)  12)";
  const char *p = exp;

  Sel_stack st{};
  st.data = arena.alloc<Sel_type>(64);
  st.size = 0;
  st.ip = 0;

  compile_expr(&p, &st);

  int result = run(&st);
  printn(result); // -> 22
}

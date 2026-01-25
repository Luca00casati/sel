#include <gc/gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sel_lexer.h"

typedef uint64_t word64;

typedef enum { OP_PUSH_INT, OP_PLUS } Sel_op;

typedef struct { Sel_op op; word64 value; } Sel_inst;

typedef struct {
    Sel_inst* code;
    size_t size;
    size_t ip;
    word64* stack;
    size_t sp;
} Sel_vm;

// ----------------- Compiler -----------------
void compile_expr(Lexer* lx, Sel_vm* vm);

void emit(Sel_vm* vm, Sel_op op, word64 value) {
    vm->code[vm->size].op = op;
    vm->code[vm->size].value = value;
    vm->size++;
}

void compile_list(Lexer* lx, Sel_vm* vm) {
    Token tok = lexer_next(lx);
    if (tok.type != TOK_SYMBOL || strncmp(tok.start, "+", tok.len) != 0) {
        fprintf(stderr, "Only + operator supported\n");
        exit(1);
    }

    int first = 1;
    while (1) {
        // Peek next token without consuming
        const char* save_cur = lx->cur;
        tok = lexer_next(lx);
        if (tok.type == TOK_RPAREN) break;

        // restore cursor so compile_expr sees the token
        lx->cur = save_cur;

        compile_expr(lx, vm);
        if (!first) emit(vm, OP_PLUS, 0);
        first = 0;
    }
}

void compile_expr(Lexer* lx, Sel_vm* vm) {
    Token tok = lexer_next(lx);
    if (tok.type == TOK_LPAREN) {
        compile_list(lx, vm);
    } else if (tok.type == TOK_INT) {
        emit(vm, OP_PUSH_INT, tok.value);
    } else {
        fprintf(stderr, "Syntax error at %d:%d\n", tok.line, tok.col);
        exit(1);
    }
}

// ----------------- VM Execution -----------------
word64 run(Sel_vm* vm) {
    vm->sp = 0;
    for (vm->ip = 0; vm->ip < vm->size; vm->ip++) {
        Sel_inst* i = &vm->code[vm->ip];
        switch (i->op) {
            case OP_PUSH_INT: vm->stack[vm->sp++] = i->value; break;
            case OP_PLUS: {
                word64 b = vm->stack[--vm->sp];
                word64 a = vm->stack[--vm->sp];
                vm->stack[vm->sp++] = a + b;
                break;
            }
        }
    }
    return vm->stack[0];
}

// ----------------- Main -----------------
int main() {
    GC_INIT();

    const char* exp = "(+ 10 2 (+ 1 5 2))";
    Lexer lx;
    lexer_init(&lx, exp, strlen(exp));

    Sel_vm vm = {0};
    vm.code = GC_MALLOC(sizeof(Sel_inst) * 64);
    vm.stack = GC_MALLOC(sizeof(word64) * 64);
    vm.size = 0;

    compile_expr(&lx, &vm);

    word64 result = run(&vm);
    printf("%lu\n", result); // -> 20
}

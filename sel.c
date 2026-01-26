// ====================== sel.c ======================
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

// --------------------------------------------------
// Common
// --------------------------------------------------
typedef int64_t word64;
#define ALIGNOF(type) offsetof(struct { char c; type member; }, member)

// --------------------------------------------------
// Arena (growable, resettable)
// --------------------------------------------------
typedef struct {
    char*  data;
    size_t size;
    size_t offset;
} Arena;

Arena* arena_create(size_t size) {
    Arena* a = malloc(sizeof(Arena));
    if (!a) { perror("arena"); exit(1); }
    a->data = malloc(size);
    if (!a->data) { perror("arena"); exit(1); }
    a->size = size;
    a->offset = 0;
    return a;
}

void arena_destroy(Arena* a) {
    if (!a) return;
    free(a->data);
    free(a);
}

void arena_reset(Arena* a) {
    a->offset = 0;
}

void* arena_alloc(Arena* a, size_t size, size_t align) {
    size_t aligned = (a->offset + (align - 1)) & ~(align - 1);
    size_t needed  = aligned + size;

    if (needed > a->size) {
        size_t new_size = a->size * 2;
        if (new_size < needed)
            new_size = needed * 2;

        char* new_data = realloc(a->data, new_size);
        if (!new_data) {
            fprintf(stderr, "arena realloc failed\n");
            exit(1);
        }

        a->data = new_data;
        a->size = new_size;
    }

    void* ptr = a->data + aligned;
    a->offset = aligned + size;
    return ptr;
}

// --------------------------------------------------
// VM
// --------------------------------------------------
typedef enum {
    OP_PUSH_INT,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV,
    OP_EQ, OP_LT, OP_GT, OP_LE, OP_GE
} Sel_op;

typedef struct {
    Sel_op op;
    word64 value;
} Sel_inst;

typedef struct {
    Sel_inst* code;
    size_t    size;
    size_t    ip;

    word64*   stack;
    size_t    sp;
} Sel_vm;

word64 run(Sel_vm* vm) {
    vm->sp = 0;

    for (vm->ip = 0; vm->ip < vm->size; vm->ip++) {
        Sel_inst i = vm->code[vm->ip];
        switch (i.op) {
            case OP_PUSH_INT:
                vm->stack[vm->sp++] = i.value;
                break;

            case OP_PLUS:
                vm->stack[vm->sp-2] += vm->stack[vm->sp-1];
                vm->sp--;
                break;

            case OP_MINUS:
                vm->stack[vm->sp-2] -= vm->stack[vm->sp-1];
                vm->sp--;
                break;

            case OP_MUL:
                vm->stack[vm->sp-2] *= vm->stack[vm->sp-1];
                vm->sp--;
                break;

            case OP_DIV:
                if (vm->stack[vm->sp-1] == 0) {
                    fprintf(stderr, "division by zero\n");
                    exit(1);
                }
                vm->stack[vm->sp-2] /= vm->stack[vm->sp-1];
                vm->sp--;
                break;

            case OP_EQ:
                vm->stack[vm->sp-2] =
                    (vm->stack[vm->sp-2] == vm->stack[vm->sp-1]);
                vm->sp--;
                break;

            case OP_LT:
                vm->stack[vm->sp-2] =
                    (vm->stack[vm->sp-2] < vm->stack[vm->sp-1]);
                vm->sp--;
                break;

            case OP_GT:
                vm->stack[vm->sp-2] =
                    (vm->stack[vm->sp-2] > vm->stack[vm->sp-1]);
                vm->sp--;
                break;

            case OP_LE:
                vm->stack[vm->sp-2] =
                    (vm->stack[vm->sp-2] <= vm->stack[vm->sp-1]);
                vm->sp--;
                break;

            case OP_GE:
                vm->stack[vm->sp-2] =
                    (vm->stack[vm->sp-2] >= vm->stack[vm->sp-1]);
                vm->sp--;
                break;

            default:
                fprintf(stderr, "bad opcode\n");
                exit(1);
        }
    }

    return vm->stack[0];
}

// --------------------------------------------------
// Compiler / Parser
// --------------------------------------------------
static void skip_ws(const char** p) {
    while (isspace(**p)) (*p)++;
}

static void emit(Sel_vm* vm, Sel_op op) {
    vm->code[vm->size++] = (Sel_inst){op, 0};
}

static void compile_expr(const char** p, Sel_vm* vm);

static void compile_list(const char** p, Sel_vm* vm) {
    skip_ws(p);

    Sel_op op;
    if      (**p == '+') op = OP_PLUS;
    else if (**p == '-') op = OP_MINUS;
    else if (**p == '*') op = OP_MUL;
    else if (**p == '/') op = OP_DIV;
    else if (**p == '=') op = OP_EQ;
    else if (**p == '<' && (*p)[1] == '=') { op = OP_LE; (*p)++; }
    else if (**p == '>' && (*p)[1] == '=') { op = OP_GE; (*p)++; }
    else if (**p == '<') op = OP_LT;
    else if (**p == '>') op = OP_GT;
    else {
        fprintf(stderr, "unknown operator\n");
        exit(1);
    }

    (*p)++; // consume operator

    // ------------------------
    // Compile operands
    // ------------------------
    skip_ws(p);
    compile_expr(p, vm); // first operand
    int argc = 1;

    while (1) {
        skip_ws(p);
        if (**p == ')') break;

        compile_expr(p, vm);
        emit(vm, op);   // left fold for non-associative
        argc++;
    }

    if (argc < 2) {
        fprintf(stderr, "operator needs at least 2 operands\n");
        exit(1);
    }

    (*p)++; // consume ')'
}

static void compile_expr(const char** p, Sel_vm* vm) {
    skip_ws(p);

    if (**p == '(') {
        (*p)++;
        compile_list(p, vm);
        return;
    }

    if (isdigit(**p)) {
        word64 v = 0;
        while (isdigit(**p)) {
            v = v * 10 + (**p - '0');
            (*p)++;
        }
        vm->code[vm->size++] = (Sel_inst){OP_PUSH_INT, v};
        return;
    }

    fprintf(stderr, "syntax error\n");
    exit(1);
}

// --------------------------------------------------
// REPL
// --------------------------------------------------
static void repl(void) {
    char line[256];
    printf("SEL REPL (Ctrl+D to exit)\n");

    Arena* arena = arena_create(1024);

    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin))
            break;

        arena_reset(arena);

        Sel_vm vm = {0};
        vm.code  = arena_alloc(arena, sizeof(Sel_inst) * 256, ALIGNOF(Sel_inst));
        vm.stack = arena_alloc(arena, sizeof(word64)   * 256, ALIGNOF(word64));

        const char* p = line;
        compile_expr(&p, &vm);
        word64 r = run(&vm);

        printf("=> %" PRIi64 "\n", r);
    }

    arena_destroy(arena);
}

// --------------------------------------------------
// Tests
// --------------------------------------------------
#ifdef TEST
static void run_test(Arena* arena, const char* expr, const char* expected) {
    arena_reset(arena);

    Sel_vm vm = {0};
    vm.code  = arena_alloc(arena, sizeof(Sel_inst) * 256, ALIGNOF(Sel_inst));
    vm.stack = arena_alloc(arena, sizeof(word64)   * 256, ALIGNOF(word64));

    const char* p = expr;
    compile_expr(&p, &vm);
    word64 r = run(&vm);

    int pass;
    if (!strcmp(expected, "T")) pass = (r != 0);
    else if (!strcmp(expected, "F")) pass = (r == 0);
    else pass = (r == atoll(expected));

    printf("[%s] %-25s => %" PRIi64 "\n",
           pass ? "PASS" : "FAIL", expr, r);
}
#endif

// --------------------------------------------------
// main
// --------------------------------------------------
int main(void) {
#ifdef TEST
    Arena* arena = arena_create(1024);

    run_test(arena, "(+ 1 2)", "3");
    run_test(arena, "(+ 1 2 3 4)", "10");
    run_test(arena, "(+ 10 (+ 1 2))", "13");
    run_test(arena, "(+ 1 2 (+ 3 4))", "10");
    run_test(arena, "(- 10 4)", "6");
    run_test(arena, "(- 20 3 2)", "15");   // ✅ fixed
    run_test(arena, "(* 2 3 4)", "24");
    run_test(arena, "(* 2 (+ 1 2))", "6");
    run_test(arena, "(/ 20 5)", "4");
    run_test(arena, "(/ 100 5 2)", "10");  // ✅ fixed
    run_test(arena, "(= 5 5)", "T");
    run_test(arena, "(= 5 6)", "F");
    run_test(arena, "(< 3 5)", "T");
    run_test(arena, "(> 3 5)", "F");
    run_test(arena, "(<= 5 5)", "T");
    run_test(arena, "(>= 6 5)", "T");

    arena_destroy(arena);
#else
    repl();
#endif
    return 0;
}

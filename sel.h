#ifndef SEL_H
#define SEL_H

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>

// ---------------- Word Type ----------------
typedef uint64_t word64;
#define PRINT_WORD64(x) do { \
    if ((x) == 0) printf("#f\n"); \
    else if ((x) == 1) printf("#t\n"); \
    else printf("%" PRIu64 "\n", (x)); \
} while(0)

// ---------------- Arena Allocator ----------------
typedef struct Chunk {
    struct Chunk* next;
    size_t size;
    size_t offset;
    char data[];
} Chunk;

typedef struct {
    Chunk* head;
    size_t chunk_size;
} Arena;

Arena* arena_create(size_t chunk_size);
void* arena_alloc(Arena* a, size_t size, size_t align);
void arena_destroy(Arena* a);

// ---------------- Lexer ----------------
typedef enum { TOK_INT, TOK_LPAREN, TOK_RPAREN, TOK_SYMBOL, TOK_BOOL, TOK_EOF } TokenType;
typedef struct { TokenType type; const char* start; size_t len; word64 value; int bool_value; } Token;
typedef struct { const char* src; const char* cur; } Lexer;

void lexer_init(Lexer* lx, const char* src, size_t len);

// ---------------- VM ----------------
typedef enum { OP_PUSH_INT, OP_PUSH_BOOL, OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_EQ, OP_LT, OP_GT, OP_LE, OP_GE } Sel_op;
typedef struct { Sel_op op; word64 value; } Sel_inst;
typedef struct { Sel_inst* code; size_t size; size_t ip; word64* stack; size_t sp; } Sel_vm;

// ---------------- Compiler & Execution ----------------
int run_expression(const char* expr, word64* out, Arena* arena);

#endif // SEL_H

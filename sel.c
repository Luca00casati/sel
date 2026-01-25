#include <gc/gc.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ---------------- Types ----------------
typedef uint64_t word64;

#define PRINT_WORD64(x) do { \
    if ((x) == 0) printf("#f\n"); \
    else if ((x) == 1) printf("#t\n"); \
    else printf("%" PRIu64 "\n", (x)); \
} while(0)

typedef enum {
    TOK_INT,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_SYMBOL,
    TOK_BOOL,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t len;
    uint64_t value;     // integer
    int bool_value;     // boolean
} Token;

typedef struct {
    const char* src;
    const char* cur;
} Lexer;

typedef enum {
    OP_PUSH_INT,
    OP_PUSH_BOOL,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV,
    OP_EQ, OP_LT, OP_GT, OP_LE, OP_GE
} Sel_op;

typedef struct { Sel_op op; word64 value; } Sel_inst;

typedef struct {
    Sel_inst* code;
    size_t size;
    size_t ip;
    word64* stack;
    size_t sp;
} Sel_vm;

// ---------------- Lexer ----------------
void lexer_init(Lexer* lx, const char* src, size_t len) {
    lx->src = src;
    lx->cur = src;
}

void skip_ws(Lexer* lx) {
    while (*lx->cur && isspace(*lx->cur)) lx->cur++;
}

Token lexer_next(Lexer* lx) {
    skip_ws(lx);
    Token tok = {0};
    tok.start = lx->cur;
    if (*lx->cur == '\0') { tok.type = TOK_EOF; return tok; }
    if (*lx->cur == '(') { lx->cur++; tok.type = TOK_LPAREN; return tok; }
    if (*lx->cur == ')') { lx->cur++; tok.type = TOK_RPAREN; return tok; }
    if (*lx->cur == '#') { // boolean
        if (lx->cur[1] == 't') { tok.type = TOK_BOOL; tok.bool_value = 1; lx->cur+=2; return tok; }
        if (lx->cur[1] == 'f') { tok.type = TOK_BOOL; tok.bool_value = 0; lx->cur+=2; return tok; }
    }
    if (isdigit(*lx->cur)) {
        uint64_t v = 0;
        while (isdigit(*lx->cur)) { v = v*10 + (*lx->cur - '0'); lx->cur++; }
        tok.type = TOK_INT; tok.value = v; return tok;
    }
    // symbol: + - * / = < > <= >=
    const char* start = lx->cur;
    while (*lx->cur && (isalnum(*lx->cur) || strchr("+-*/=<>&",*lx->cur))) lx->cur++;
    tok.type = TOK_SYMBOL;
    tok.start = start;
    tok.len = lx->cur - start;
    return tok;
}

// ---------------- VM ----------------
void emit(Sel_vm* vm, Sel_op op, word64 value) {
    vm->code[vm->size].op = op;
    vm->code[vm->size].value = value;
    vm->size++;
}

Sel_op symbol_to_op(const char* sym, size_t len) {
    if (len == 1) {
        switch (sym[0]) {
            case '+': return OP_PLUS;
            case '-': return OP_MINUS;
            case '*': return OP_MUL;
            case '/': return OP_DIV;
            case '=': return OP_EQ;
            case '<': return OP_LT;
            case '>': return OP_GT;
        }
    } else if (len == 2) {
        if (strncmp(sym,"<=",2)==0) return OP_LE;
        if (strncmp(sym,">=",2)==0) return OP_GE;
    }
    fprintf(stderr,"Unknown operator: %.*s\n",(int)len,sym);
    exit(1);
}

// ---------------- Compiler ----------------
void compile_expr(Lexer* lx, Sel_vm* vm);

void compile_list(Lexer* lx, Sel_vm* vm) {
    Token tok = lexer_next(lx);
    if (tok.type != TOK_SYMBOL) { fprintf(stderr,"Expected operator\n"); exit(1); }
    Sel_op op = symbol_to_op(tok.start, tok.len);

    int first = 1;
    while (1) {
        const char* save = lx->cur;
        tok = lexer_next(lx);
        if (tok.type == TOK_RPAREN) break;
        lx->cur = save; // restore
        compile_expr(lx, vm);
        if (!first) emit(vm, op, 0);
        first = 0;
    }
}

void compile_expr(Lexer* lx, Sel_vm* vm) {
    Token tok = lexer_next(lx);
    if (tok.type == TOK_LPAREN) { compile_list(lx, vm); }
    else if (tok.type == TOK_INT) emit(vm, OP_PUSH_INT, tok.value);
    else if (tok.type == TOK_BOOL) emit(vm, OP_PUSH_BOOL, tok.bool_value);
    else { fprintf(stderr,"Syntax error\n"); exit(1); }
}

// ---------------- VM Execution ----------------
word64 run(Sel_vm* vm) {
    vm->sp = 0;
    for (vm->ip=0; vm->ip<vm->size; vm->ip++) {
        Sel_inst* i = &vm->code[vm->ip];
        switch(i->op) {
            case OP_PUSH_INT: vm->stack[vm->sp++] = i->value; break;
            case OP_PUSH_BOOL: vm->stack[vm->sp++] = i->value; break;
            case OP_PLUS: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=a+b; break; }
            case OP_MINUS:{ word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=a-b; break; }
            case OP_MUL: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=a*b; break; }
            case OP_DIV: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; if(b==0){fprintf(stderr,"Div0\n");exit(1);} vm->stack[vm->sp++]=a/b; break; }
            case OP_EQ: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a==b)?1:0; break; }
            case OP_LT: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a<b)?1:0; break; }
            case OP_GT: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a>b)?1:0; break; }
            case OP_LE: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a<=b)?1:0; break; }
            case OP_GE: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a>=b)?1:0; break; }
        }
    }
    return vm->stack[0];
}

// ---------------- Helper for tests ----------------
int run_expression(const char* expr, uint64_t* out) {
    if(!expr||!out) return -1;
    Lexer lx; lexer_init(&lx, expr, strlen(expr));
    Sel_vm vm={0};
    vm.code = GC_MALLOC(sizeof(Sel_inst)*256);
    vm.stack = GC_MALLOC(sizeof(word64)*256);
    vm.size=0;
    compile_expr(&lx,&vm);
    *out = run(&vm);
    return 0;
}

// ---------------- REPL ----------------
#ifndef TEST_BUILD
int main() {
    GC_INIT();
    printf("Tiny Lisp REPL (supports + - * / = < > <= >= #t #f)\nType 'exit' to quit.\n");
    char buffer[1024];
    while(1){
        printf("> ");
        if(!fgets(buffer,sizeof(buffer),stdin)) break;
        if(strncmp(buffer,"exit",4)==0) break;
        uint64_t result;
        if(run_expression(buffer,&result)==0) PRINT_WORD64(result);
    }
    return 0;
}
#endif

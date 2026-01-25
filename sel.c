#include "sel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>

// ---------------- Arena ----------------
Arena* arena_create(size_t chunk_size){
    Arena* a = malloc(sizeof(Arena));
    if(!a){ fprintf(stderr,"arena malloc failed\n"); exit(1); }
    a->head = NULL;
    a->chunk_size = chunk_size ? chunk_size : 4096;
    return a;
}

void* arena_alloc(Arena* a, size_t size, size_t align){
    if(!a->head || a->head->offset + size + align > a->head->size){
        size_t chunk_size = (size+align > a->chunk_size) ? size+align : a->chunk_size;
        Chunk* c = malloc(sizeof(Chunk)+chunk_size);
        if(!c){ fprintf(stderr,"arena chunk malloc failed\n"); exit(1); }
        c->next = a->head;
        c->size = chunk_size;
        c->offset = 0;
        a->head = c;
    }
    uintptr_t ptr = (uintptr_t)(a->head->data + a->head->offset);
    uintptr_t aligned = (ptr + align - 1) & ~(align - 1);
    a->head->offset = (aligned - (uintptr_t)a->head->data) + size;
    return (void*)aligned;
}

void arena_destroy(Arena* a){
    Chunk* c = a->head;
    while(c){
        Chunk* next = c->next;
        free(c);
        c = next;
    }
    free(a);
}

// ---------------- Lexer ----------------
void lexer_init(Lexer* lx, const char* src, size_t len){ (void)len; lx->src=src; lx->cur=src; }

static void skip_ws(Lexer* lx){ while(*lx->cur && isspace(*lx->cur)) lx->cur++; }

static Token lexer_next(Lexer* lx){
    skip_ws(lx);
    Token tok = {0};
    tok.start = lx->cur;
    if(!*lx->cur){ tok.type=TOK_EOF; return tok; }
    if(*lx->cur=='('){ lx->cur++; tok.type=TOK_LPAREN; return tok; }
    if(*lx->cur==')'){ lx->cur++; tok.type=TOK_RPAREN; return tok; }
    if(*lx->cur=='#'){
        if(lx->cur[1]=='t'){ tok.type=TOK_BOOL; tok.bool_value=1; lx->cur+=2; return tok; }
        if(lx->cur[1]=='f'){ tok.type=TOK_BOOL; tok.bool_value=0; lx->cur+=2; return tok; }
    }
    if(isdigit(*lx->cur)){
        word64 v=0; while(isdigit(*lx->cur)){ v=v*10+(*lx->cur-'0'); lx->cur++; }
        tok.type=TOK_INT; tok.value=v; return tok;
    }
    const char* start = lx->cur;
    while(*lx->cur && (isalnum(*lx->cur) || strchr("+-*/=<>&",*lx->cur))) lx->cur++;
    tok.type=TOK_SYMBOL; tok.start=start; tok.len=lx->cur-start;
    return tok;
}

// ---------------- VM ----------------
static void emit(Sel_vm* vm, Sel_op op, word64 value){ vm->code[vm->size].op=op; vm->code[vm->size].value=value; vm->size++; }

static Sel_op symbol_to_op(const char* sym, size_t len){
    if(len==1){ switch(sym[0]){ case '+': return OP_PLUS; case '-': return OP_MINUS; case '*': return OP_MUL; case '/': return OP_DIV; case '=': return OP_EQ; case '<': return OP_LT; case '>': return OP_GT; } }
    else if(len==2){ if(strncmp(sym,"<=",2)==0) return OP_LE; if(strncmp(sym,">=",2)==0) return OP_GE; }
    fprintf(stderr,"Unknown operator: %.*s\n",(int)len,sym); exit(1);
}

// ---------------- Compiler ----------------
static void compile_expr(Lexer* lx, Sel_vm* vm, Arena* arena);

static void compile_list(Lexer* lx, Sel_vm* vm, Arena* arena){
    Token tok = lexer_next(lx);
    if(tok.type!=TOK_SYMBOL){ fprintf(stderr,"Expected operator\n"); exit(1); }
    Sel_op op = symbol_to_op(tok.start,tok.len);
    int first=1;
    while(1){
        const char* save=lx->cur;
        tok=lexer_next(lx);
        if(tok.type==TOK_RPAREN) break;
        lx->cur=save;
        compile_expr(lx,vm,arena);
        if(!first) emit(vm,op,0);
        first=0;
    }
}

static void compile_expr(Lexer* lx, Sel_vm* vm, Arena* arena){
    Token tok = lexer_next(lx);
    if(tok.type==TOK_LPAREN) compile_list(lx,vm,arena);
    else if(tok.type==TOK_INT) emit(vm,OP_PUSH_INT,tok.value);
    else if(tok.type==TOK_BOOL) emit(vm,OP_PUSH_BOOL,tok.bool_value);
    else { fprintf(stderr,"Syntax error\n"); exit(1); }
}

// ---------------- VM Execution ----------------
static word64 run_vm(Sel_vm* vm){
    vm->sp=0;
    for(vm->ip=0; vm->ip<vm->size; vm->ip++){
        Sel_inst* i=&vm->code[vm->ip];
        switch(i->op){
            case OP_PUSH_INT: vm->stack[vm->sp++]=i->value; break;
            case OP_PUSH_BOOL: vm->stack[vm->sp++]=i->value; break;
            case OP_PLUS: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=a+b; break; }
            case OP_MINUS:{ word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=a-b; break; }
            case OP_MUL: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=a*b; break; }
            case OP_DIV: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; if(!b){fprintf(stderr,"Div0\n");exit(1);} vm->stack[vm->sp++]=a/b; break; }
            case OP_EQ: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a==b)?1:0; break; }
            case OP_LT: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a<b)?1:0; break; }
            case OP_GT: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a>b)?1:0; break; }
            case OP_LE: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a<=b)?1:0; break; }
            case OP_GE: { word64 b=vm->stack[--vm->sp]; word64 a=vm->stack[--vm->sp]; vm->stack[vm->sp++]=(a>=b)?1:0; break; }
        }
    }
    return vm->stack[0];
}

// ---------------- Public API ----------------
int run_expression(const char* expr, word64* out, Arena* arena){
    if(!expr||!out) return -1;
    Lexer lx; lexer_init(&lx,expr,strlen(expr));
    Sel_vm vm={0};
    vm.code = arena_alloc(arena,sizeof(Sel_inst)*256,_Alignof(Sel_inst));
    vm.stack=arena_alloc(arena,sizeof(word64)*256,_Alignof(word64));
    vm.size=0;
    compile_expr(&lx,&vm,arena);
    *out = run_vm(&vm);
    return 0;
}

// ---------------- REPL ----------------
#ifndef TEST_BUILD
int main(void){
    Arena* arena = arena_create(4096);
    printf("Tiny Lisp REPL (supports + - * / = < > <= >= #t #f)\nType 'exit' to quit.\n");
    char buffer[1024];
    while(1){
        printf("> ");
        if(!fgets(buffer,sizeof(buffer),stdin)) break;
        if(strncmp(buffer,"exit",4)==0) break;
        word64 result;
        Arena* expr_arena = arena_create(1024); // per expression
        if(run_expression(buffer,&result,expr_arena)==0) PRINT_WORD64(result);
        arena_destroy(expr_arena);
    }
    arena_destroy(arena);
    return 0;
}
#endif

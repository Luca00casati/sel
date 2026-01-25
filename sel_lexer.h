#ifndef SEL_LEXER_H
#define SEL_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_INT,
    TOK_SYMBOL,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start; // pointer to start of lexeme
    size_t len;        // length of lexeme
    long value;        // only for TOK_INT
    int line, col;
} Token;

typedef struct {
    const char* cur;
    const char* end;
    int line;
    int col;
} Lexer;

void lexer_init(Lexer* lx, const char* src, size_t len);
Token lexer_next(Lexer* lx);

#endif

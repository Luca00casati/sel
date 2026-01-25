#include "sel_lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lexer_init(Lexer* lx, const char* src, size_t len) {
    lx->cur = src;
    lx->end = src + len;
    lx->line = 1;
    lx->col = 1;
}

Token lexer_next(Lexer* lx) {
    Token tok = {0};
    tok.line = lx->line;
    tok.col = lx->col;

    // Skip whitespace
    while (lx->cur < lx->end && isspace((unsigned char)*lx->cur)) {
        if (*lx->cur == '\n') {
            lx->line++;
            lx->col = 1;
        } else {
            lx->col++;
        }
        lx->cur++;
    }

    if (lx->cur >= lx->end) {
        tok.type = TOK_EOF;
        return tok;
    }

    char c = *lx->cur;

    if (c == '(') {
        tok.type = TOK_LPAREN;
        tok.start = lx->cur;
        tok.len = 1;
        lx->cur++;
        lx->col++;
        return tok;
    }

    if (c == ')') {
        tok.type = TOK_RPAREN;
        tok.start = lx->cur;
        tok.len = 1;
        lx->cur++;
        lx->col++;
        return tok;
    }

    if (isdigit((unsigned char)c)) {
        tok.type = TOK_INT;
        tok.start = lx->cur;
        tok.value = 0;
        while (lx->cur < lx->end && isdigit((unsigned char)*lx->cur)) {
            tok.value = tok.value * 10 + (*lx->cur - '0');
            lx->cur++;
            lx->col++;
        }
        tok.len = lx->cur - tok.start;
        return tok;
    }

    if (isalpha((unsigned char)c) || strchr("_+-*/", c)) {
        tok.type = TOK_SYMBOL;
        tok.start = lx->cur;
        while (lx->cur < lx->end &&
               (isalnum((unsigned char)*lx->cur) || strchr("_+-*/", *lx->cur))) {
            lx->cur++;
            lx->col++;
        }
        tok.len = lx->cur - tok.start;
        return tok;
    }

    // Unknown character
    fprintf(stderr, "Lexer error at %d:%d: unexpected '%c'\n", lx->line, lx->col, c);
    exit(1);
}

#ifndef LEXER_H
#define LEXER_H

#include "base/string8.h"

typedef struct {
    String source;
    ssize index;
    ssize token_begin_index;
} Lexer;

typedef enum {
    TOK_EOF,
    TOK_INTEGER,
    TOK_STRING,
    TOK_IDENTIFIER,
    TOK_LEFT_BRACE,
    TOK_RIGHT_BRACE,
    TOK_LEFT_BRACKET,
    TOK_RIGHT_BRACKET,
    TOK_COLON,
    TOK_EQUAL,
    TOK_COMMA,
    TOK_SEMICOLON,
} TokenKind;

typedef struct {
    TokenKind kind;
    String lexeme;
    ssize source_index;
} Token;

Token lexer_next_token(Lexer *l);

#endif // LEXER_H

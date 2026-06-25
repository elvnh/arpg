#include "lexer.h"

static b32 lexer_is_at_end(Lexer *l)
{
    ASSERT(l->index <= l->source.length);

    b32 result = l->index == l->source.length;

    return result;
}

static char lexer_peek_char(Lexer *l)
{
    ASSERT(!lexer_is_at_end(l));
    char result = l->source.data[l->index];

    return result;
}

static char lexer_advance_char(Lexer *l)
{
    char result = lexer_peek_char(l);
    ++l->index;

    return result;
}

static void lexer_skip_whitespace(Lexer *l)
{
    while (!lexer_is_at_end(l)) {
        char c = lexer_peek_char(l);

        if (!is_whitespace(c)) {
            break;
        } else {
            lexer_advance_char(l);
        }
    }
}

static TokenKind lexer_tokenize_string(Lexer *l)
{
    TokenKind result = 0;

    while (!lexer_is_at_end(l)) {
        char c = lexer_advance_char(l);

        if (c == '"') {
            result = TOK_STRING;
            break;
        }

        lexer_advance_char(l);
    }

    return result;
}

static TokenKind lexer_tokenize_number(Lexer *l)
{
    while (!lexer_is_at_end(l)) {
        char c = lexer_peek_char(l);

        if (!is_digit(c)) {
            break;
        }

        lexer_advance_char(l);
    }

    TokenKind result = TOK_INTEGER;
    return result;
}

static TokenKind lexer_tokenize_identifier(Lexer *l)
{
    while (!lexer_is_at_end(l)) {
        char c = lexer_peek_char(l);

        if (!is_alphanumeric(c)) {
            break;
        }

        lexer_advance_char(l);
    }

    TokenKind result = TOK_IDENTIFIER;
    return result;
}

Token lexer_next_token(Lexer *l)
{
    Token result = {0};

    lexer_skip_whitespace(l);

    if (!lexer_is_at_end(l)) {
        l->token_begin_index = l->index;

        char c = lexer_advance_char(l);
        TokenKind kind = 0;

        switch (c) {
            case '{': {
                kind = TOK_LEFT_BRACE;
            } break;

            case '}': {
                kind = TOK_RIGHT_BRACE;
            } break;

            case '[': {
                kind = TOK_LEFT_BRACKET;
            } break;

            case ']': {
                kind = TOK_RIGHT_BRACKET;
            } break;

            case ':': {
                kind = TOK_COLON;
            } break;

            case '"': {
                kind = lexer_tokenize_string(l);
            } break;

            case ',': {
                kind = TOK_COMMA;
            } break;

            case ';': {
                kind = TOK_SEMICOLON;
            } break;

            case '=': {
                kind = TOK_EQUAL;
            } break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                kind = lexer_tokenize_number(l);
            } break;

            default: {
                if (is_alpha(c)) {
                    kind = lexer_tokenize_identifier(l);
                }
            } break;
        }

        ssize length = l->index - l->token_begin_index;
        String lexeme = str_create_span(l->source, l->token_begin_index, length);
        result = (Token){kind, lexeme, l->token_begin_index};
    }

    return result;
}

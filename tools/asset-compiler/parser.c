#include "parser.h"

#include "base/int_parse.h"
#include "base/linear_arena.h"
#include "base/sl_list.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
    ValueList nodes;
    Token current_token;
    LinearArena *arena;
} Parser;

static Token parser_peek(Parser *p)
{
    Token result = p->current_token;

    return result;
}

static Token parser_scan(Parser *p)
{
    p->current_token = lexer_next_token(&p->lexer);

    Token result = p->current_token;

    return result;
}

static b32 parser_check(Parser *p, TokenKind kind)
{
    Token current = parser_peek(p);
    b32 result = current.kind == kind;

    return result;
}

static b32 parser_match(Parser *p, TokenKind kind)
{
    b32 result = parser_check(p, kind);

    if (result) {
        parser_scan(p);
    }

    return result;
}

static void parser_expect(Parser *p, TokenKind tok)
{
    if (!parser_match(p, tok)) {
        // TODO: proper errors
        ASSERT(0 && "Did not receive expected token");
    }
}

static b32 parser_is_at_end(Parser *p)
{
    b32 result = parser_peek(p).kind == TOK_EOF;

    return result;
}

static Value *parse_value(Parser *p);

static Attribute *parse_attribute(Parser *p)
{
    Token name = parser_peek(p);
    parser_scan(p);

    ASSERT(name.kind == TOK_IDENTIFIER);

    Attribute *result = la_allocate_item(p->arena, Attribute);
    result->name = name.lexeme;

    parser_expect(p, TOK_EQUAL);

    Value *value = parse_value(p);
    result->value = value;

    return result;
}

static Value *parse_record(Parser *p)
{
    Value *result = value_make(p->arena, VALUE_RECORD);

    do {
        if (parser_check(p, TOK_RIGHT_BRACE)) {
            break;
        }

        Attribute *attr = parse_attribute(p);

        sl_list_push_back(&result->as.record, attr);
    } while (parser_match(p, TOK_COMMA));

    if (parser_is_at_end(p)) {
        ASSERT(0 && "Unterminated record");
    } else {
        parser_expect(p, TOK_RIGHT_BRACE);
    }

    return 0;
}

static Value *parse_list(Parser *p)
{
    Value *result = value_make(p->arena, VALUE_LIST);

    do {
        if (parser_check(p, TOK_RIGHT_BRACKET)) {
            break;
        }

        Value *elem = parse_value(p);
        sl_list_push_back(&result->as.list, elem);
    } while (parser_match(p, TOK_COMMA));

    if (parser_is_at_end(p)) {
        ASSERT(0 && "Unterminated list");
    }

    if (!parser_match(p, TOK_RIGHT_BRACKET)) {
        ASSERT(0 && "Expected list end");
    }

    return result;
}

static Value *parse_value(Parser *p)
{
    Value *node = 0;

    Token current_token = parser_peek(p);
    parser_scan(p);

    switch (current_token.kind) {
        case TOK_INTEGER: {
            node = value_make(p->arena, VALUE_INT);

            MaybeS64 int_opt = parse_s64(current_token.lexeme, NUM_BASE_DEC);

            if (int_opt.ok) {
                node->as.integer = int_opt.value;
            } else {
                ASSERT(0);
            }
        } break;

        case TOK_STRING: {
            node = value_make(p->arena, VALUE_STRING);

            node->as.string = current_token.lexeme;
        } break;

        case TOK_IDENTIFIER: {
            node = value_make(p->arena, VALUE_IDENTIFIER);

            node->as.identifier = current_token.lexeme;
        } break;

        case TOK_LEFT_BRACE: {
            node = parse_record(p);
        } break;

        case TOK_LEFT_BRACKET: {
            node = parse_list(p);
        } break;

        default: {
            ASSERT(0);
        } break;
    }

    return node;
}

Value *parse(String source, LinearArena *arena)
{
    Parser p = {
        .lexer = {source, 0},
        .arena = arena,
    };

    parser_scan(&p);

    // The root of the file is an implicit record
    Value *result = value_make(arena, VALUE_RECORD);

    while (!parser_is_at_end(&p)) {
        if (parser_check(&p, TOK_IDENTIFIER)) {
            Attribute *attr = parse_attribute(&p);
            ASSERT(attr);

            sl_list_push_back(&result->as.record, attr);
        } else {
            ASSERT(0);
        }
    }

    return result;
}

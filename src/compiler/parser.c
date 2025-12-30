/**
 * MTPScript Parser Implementation
 * Based on TECHSPECV5.md §4.2
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#include "parser.h"
#include <string.h>

mtpscript_parser_t *mtpscript_parser_new(mtpscript_vector_t *tokens) {
    mtpscript_parser_t *parser = MTPSCRIPT_MALLOC(sizeof(mtpscript_parser_t));
    parser->tokens = tokens;
    parser->position = 0;
    return parser;
}

void mtpscript_parser_free(mtpscript_parser_t *parser) {
    if (parser) MTPSCRIPT_FREE(parser);
}

static mtpscript_token_t *peek_token(mtpscript_parser_t *parser) {
    return mtpscript_vector_get(parser->tokens, parser->position);
}

static mtpscript_token_t *advance_token(mtpscript_parser_t *parser) {
    mtpscript_token_t *token = peek_token(parser);
    if (token->type != MTPSCRIPT_TOKEN_EOF) {
        parser->position++;
    }
    return token;
}

static bool check_token(mtpscript_parser_t *parser, mtpscript_token_type_t type) {
    return peek_token(parser)->type == type;
}

static bool match_token(mtpscript_parser_t *parser, mtpscript_token_type_t type) {
    if (check_token(parser, type)) {
        advance_token(parser);
        return true;
    }
    return false;
}

static mtpscript_type_t *parse_type(mtpscript_parser_t *parser) {
    mtpscript_token_t *token = advance_token(parser);
    mtpscript_type_t *type;
    if (strcmp(mtpscript_string_cstr(token->lexeme), "Int") == 0) {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_INT);
    } else if (strcmp(mtpscript_string_cstr(token->lexeme), "String") == 0) {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_STRING);
    } else {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_CUSTOM);
        type->name = mtpscript_string_from_cstr(mtpscript_string_cstr(token->lexeme));
    }
    return type;
}

static mtpscript_expression_t *parse_expression(mtpscript_parser_t *parser) {
    mtpscript_token_t *token = advance_token(parser);
    mtpscript_expression_t *expr;
    if (token->type == MTPSCRIPT_TOKEN_INT) {
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_INT_LITERAL);
        expr->data.int_val = atoll(mtpscript_string_cstr(token->lexeme));
    } else if (token->type == MTPSCRIPT_TOKEN_IDENTIFIER) {
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_VARIABLE);
        expr->data.variable.name = mtpscript_string_from_cstr(mtpscript_string_cstr(token->lexeme));
    } else {
        // Fallback
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_INT_LITERAL);
    }
    return expr;
}

static mtpscript_statement_t *parse_statement(mtpscript_parser_t *parser) {
    if (match_token(parser, MTPSCRIPT_TOKEN_RETURN)) {
        mtpscript_statement_t *stmt = mtpscript_statement_new(MTPSCRIPT_STMT_RETURN_STMT);
        stmt->data.return_stmt.expression = parse_expression(parser);
        return stmt;
    }
    // Fallback
    mtpscript_statement_t *stmt = mtpscript_statement_new(MTPSCRIPT_STMT_EXPRESSION_STMT);
    stmt->data.expression_stmt.expression = parse_expression(parser);
    return stmt;
}

static mtpscript_declaration_t *parse_declaration(mtpscript_parser_t *parser) {
    if (match_token(parser, MTPSCRIPT_TOKEN_FUNC)) {
        mtpscript_declaration_t *decl = mtpscript_declaration_new(MTPSCRIPT_DECL_FUNCTION);
        mtpscript_token_t *name = advance_token(parser);
        decl->data.function.name = mtpscript_string_from_cstr(mtpscript_string_cstr(name->lexeme));

        match_token(parser, MTPSCRIPT_TOKEN_LPAREN);
        decl->data.function.params = mtpscript_vector_new();
        while (!check_token(parser, MTPSCRIPT_TOKEN_RPAREN)) {
            mtpscript_param_t *param = MTPSCRIPT_MALLOC(sizeof(mtpscript_param_t));
            param->name = mtpscript_string_from_cstr(mtpscript_string_cstr(advance_token(parser)->lexeme));
            match_token(parser, MTPSCRIPT_TOKEN_COLON);
            param->type = parse_type(parser);
            mtpscript_vector_push(decl->data.function.params, param);
            if (!match_token(parser, MTPSCRIPT_TOKEN_COMMA)) break;
        }
        match_token(parser, MTPSCRIPT_TOKEN_RPAREN);

        if (match_token(parser, MTPSCRIPT_TOKEN_COLON)) {
            decl->data.function.return_type = parse_type(parser);
        }

        match_token(parser, MTPSCRIPT_TOKEN_LBRACE);
        decl->data.function.body = mtpscript_vector_new();
        while (!check_token(parser, MTPSCRIPT_TOKEN_RBRACE)) {
            mtpscript_vector_push(decl->data.function.body, parse_statement(parser));
        }
        match_token(parser, MTPSCRIPT_TOKEN_RBRACE);
        return decl;
    }
    return NULL;
}

mtpscript_error_t *mtpscript_parser_parse(mtpscript_parser_t *parser, mtpscript_program_t **program_out) {
    mtpscript_program_t *program = mtpscript_program_new();
    *program_out = program;

    while (peek_token(parser)->type != MTPSCRIPT_TOKEN_EOF) {
        mtpscript_declaration_t *decl = parse_declaration(parser);
        if (decl) {
            mtpscript_vector_push(program->declarations, decl);
        } else {
            advance_token(parser);
        }
    }
    return NULL;
}

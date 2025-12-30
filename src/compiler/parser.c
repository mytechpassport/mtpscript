/**
 * MTPScript Parser Implementation
 * Specification §4.2
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "parser.h"
#include <string.h>
#include <stdio.h>

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
    // printf("DEBUG: parse_type sees token: %s\n", mtpscript_string_cstr(token->lexeme));

    if (strcmp(mtpscript_string_cstr(token->lexeme), "Int") == 0) {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_INT);
    } else if (strcmp(mtpscript_string_cstr(token->lexeme), "String") == 0) {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_STRING);
    } else if (strcmp(mtpscript_string_cstr(token->lexeme), "Bool") == 0) {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_BOOL);
    } else if (strcmp(mtpscript_string_cstr(token->lexeme), "Decimal") == 0) {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_DECIMAL);
    } else if (strcmp(mtpscript_string_cstr(token->lexeme), "Option") == 0) {
        // TODO: Implement Option<T> parsing
        type = mtpscript_type_new(MTPSCRIPT_TYPE_CUSTOM);
        type->name = mtpscript_string_from_cstr("Option");
    } else if (strcmp(mtpscript_string_cstr(token->lexeme), "Result") == 0) {
        // TODO: Implement Result<T,E> parsing
        type = mtpscript_type_new(MTPSCRIPT_TYPE_CUSTOM);
        type->name = mtpscript_string_from_cstr("Result");
    } else {
        type = mtpscript_type_new(MTPSCRIPT_TYPE_CUSTOM);
        type->name = mtpscript_string_from_cstr(mtpscript_string_cstr(token->lexeme));
    }
    return type;
}

static mtpscript_expression_t *parse_primary_expression(mtpscript_parser_t *parser) {
    mtpscript_token_t *token;

    // Check for await
    if (check_token(parser, MTPSCRIPT_TOKEN_IDENTIFIER) &&
        strcmp(mtpscript_string_cstr(peek_token(parser)->lexeme), "await") == 0) {
        advance_token(parser); // consume 'await'
        mtpscript_expression_t *await_expr = mtpscript_expression_new(MTPSCRIPT_EXPR_AWAIT_EXPR);
        await_expr->data.await.expression = parse_primary_expression(parser);
        return await_expr;
    }

    token = advance_token(parser);
    mtpscript_expression_t *expr;
    if (token->type == MTPSCRIPT_TOKEN_INT) {
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_INT_LITERAL);
        expr->data.int_val = atoll(mtpscript_string_cstr(token->lexeme));
    } else if (token->type == MTPSCRIPT_TOKEN_DECIMAL) {
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_DECIMAL_LITERAL);
        expr->data.decimal_val = mtpscript_string_from_cstr(mtpscript_string_cstr(token->lexeme));
    } else if (token->type == MTPSCRIPT_TOKEN_BOOL) {
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_BOOL_LITERAL);
        expr->data.bool_val = strcmp(mtpscript_string_cstr(token->lexeme), "true") == 0;
    } else if (token->type == MTPSCRIPT_TOKEN_IDENTIFIER) {
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_VARIABLE);
        expr->data.variable.name = mtpscript_string_from_cstr(mtpscript_string_cstr(token->lexeme));
    } else {
        // Fallback
        expr = mtpscript_expression_new(MTPSCRIPT_EXPR_INT_LITERAL);
    }
    return expr;
}

static mtpscript_expression_t *parse_expression(mtpscript_parser_t *parser) {
    mtpscript_expression_t *expr = parse_primary_expression(parser);

    // Handle binary operators
    if (check_token(parser, MTPSCRIPT_TOKEN_STAR) ||
        check_token(parser, MTPSCRIPT_TOKEN_PLUS) ||
        check_token(parser, MTPSCRIPT_TOKEN_MINUS) ||
        check_token(parser, MTPSCRIPT_TOKEN_SLASH)) {
        mtpscript_token_t *op_token = advance_token(parser);
        mtpscript_expression_t *binary_expr = mtpscript_expression_new(MTPSCRIPT_EXPR_BINARY_EXPR);
        binary_expr->data.binary.left = expr;
        binary_expr->data.binary.op = mtpscript_string_cstr(op_token->lexeme);
        binary_expr->data.binary.right = parse_expression(parser);
        expr = binary_expr;
    }

    // Handle pipeline operators (left-associative)
    while (match_token(parser, MTPSCRIPT_TOKEN_PIPE)) {
        mtpscript_expression_t *pipe_expr = mtpscript_expression_new(MTPSCRIPT_EXPR_PIPE_EXPR);
        pipe_expr->data.pipe.left = expr;
        pipe_expr->data.pipe.right = parse_primary_expression(parser);
        expr = pipe_expr;
    }

    return expr;
}

static mtpscript_statement_t *parse_statement(mtpscript_parser_t *parser) {
    if (match_token(parser, MTPSCRIPT_TOKEN_RETURN)) {
        mtpscript_statement_t *stmt = mtpscript_statement_new(MTPSCRIPT_STMT_RETURN_STMT);
        stmt->data.return_stmt.expression = parse_expression(parser);
        return stmt;
    } else if (match_token(parser, MTPSCRIPT_TOKEN_LET)) {
        mtpscript_statement_t *stmt = mtpscript_statement_new(MTPSCRIPT_STMT_VAR_DECL);
        mtpscript_token_t *name_token = advance_token(parser);
        stmt->data.var_decl.name = mtpscript_string_from_cstr(mtpscript_string_cstr(name_token->lexeme));
        match_token(parser, MTPSCRIPT_TOKEN_EQUALS);
        stmt->data.var_decl.initializer = parse_expression(parser);
        return stmt;
    }
    // Fallback
    mtpscript_statement_t *stmt = mtpscript_statement_new(MTPSCRIPT_STMT_EXPRESSION_STMT);
    stmt->data.expression_stmt.expression = parse_expression(parser);
    return stmt;
}

static mtpscript_declaration_t *parse_declaration(mtpscript_parser_t *parser) {
    if (0 && match_token(parser, MTPSCRIPT_TOKEN_API)) { // TODO: Implement API parsing
        mtpscript_declaration_t *decl = mtpscript_declaration_new(MTPSCRIPT_DECL_API);

        // Parse HTTP method - should be GET, POST, etc.
        mtpscript_token_t *method_token = advance_token(parser);
        if (method_token->type < MTPSCRIPT_TOKEN_GET || method_token->type > MTPSCRIPT_TOKEN_DELETE) {
            // For now, accept any identifier as method
            decl->data.api.method = mtpscript_string_from_cstr(mtpscript_string_cstr(method_token->lexeme));
        } else {
            decl->data.api.method = mtpscript_string_from_cstr(mtpscript_string_cstr(method_token->lexeme));
        }

        // Parse path (string literal)
        mtpscript_token_t *path_token = advance_token(parser);
        if (path_token->type != MTPSCRIPT_TOKEN_STRING) {
            // Error handling
            return NULL;
        }
        decl->data.api.path = mtpscript_string_from_cstr(mtpscript_string_cstr(path_token->lexeme));

        // Parse effects (optional)
        mtpscript_vector_t *effects = mtpscript_vector_new();
        if (match_token(parser, MTPSCRIPT_TOKEN_USES)) {
            match_token(parser, MTPSCRIPT_TOKEN_LBRACE);
            while (!check_token(parser, MTPSCRIPT_TOKEN_RBRACE)) {
                mtpscript_token_t *effect_token = advance_token(parser);
                mtpscript_string_t *effect_name = mtpscript_string_from_cstr(mtpscript_string_cstr(effect_token->lexeme));
                mtpscript_vector_push(effects, effect_name);
                if (!match_token(parser, MTPSCRIPT_TOKEN_COMMA)) break;
            }
            match_token(parser, MTPSCRIPT_TOKEN_RBRACE);
        }

        // Parse function handler
        if (!match_token(parser, MTPSCRIPT_TOKEN_FUNC)) {
            // Error: expected func after api declaration
            return NULL;
        }

        // Create function declaration for handler
        mtpscript_function_decl_t *func = MTPSCRIPT_MALLOC(sizeof(mtpscript_function_decl_t));
        mtpscript_token_t *name = advance_token(parser);
        func->name = mtpscript_string_from_cstr(mtpscript_string_cstr(name->lexeme));

        match_token(parser, MTPSCRIPT_TOKEN_LPAREN);
        func->params = mtpscript_vector_new();
        while (!check_token(parser, MTPSCRIPT_TOKEN_RPAREN)) {
            mtpscript_param_t *param = MTPSCRIPT_MALLOC(sizeof(mtpscript_param_t));
            param->name = mtpscript_string_from_cstr(mtpscript_string_cstr(advance_token(parser)->lexeme));
            match_token(parser, MTPSCRIPT_TOKEN_COLON);
            param->type = parse_type(parser);
            mtpscript_vector_push(func->params, param);
            if (!match_token(parser, MTPSCRIPT_TOKEN_COMMA)) break;
        }
        match_token(parser, MTPSCRIPT_TOKEN_RPAREN);

        if (match_token(parser, MTPSCRIPT_TOKEN_COLON)) {
            func->return_type = parse_type(parser);
        }

        match_token(parser, MTPSCRIPT_TOKEN_LBRACE);
        func->body = mtpscript_vector_new();
        while (!check_token(parser, MTPSCRIPT_TOKEN_RBRACE)) {
            mtpscript_vector_push(func->body, parse_statement(parser));
        }
        match_token(parser, MTPSCRIPT_TOKEN_RBRACE);

        func->effects = effects;
        decl->data.api.handler = func;

        return decl;
    } else if (match_token(parser, MTPSCRIPT_TOKEN_FUNC)) {
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
        decl->data.function.effects = mtpscript_vector_new(); // Empty for now
        return decl;
    } else if (match_token(parser, MTPSCRIPT_TOKEN_FUNC)) {
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

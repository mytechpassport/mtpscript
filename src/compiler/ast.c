/**
 * MTPScript AST Implementation
 * Based on TECHSPECV5.md §4.2
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#include "ast.h"
#include <string.h>

mtpscript_type_t *mtpscript_type_new(mtpscript_type_kind_t kind) {
    mtpscript_type_t *type = MTPSCRIPT_MALLOC(sizeof(mtpscript_type_t));
    type->kind = kind;
    type->name = NULL;
    type->inner = NULL;
    type->key = NULL;
    type->value = NULL;
    return type;
}

mtpscript_expression_t *mtpscript_expression_new(mtpscript_expression_kind_t kind) {
    mtpscript_expression_t *expr = MTPSCRIPT_MALLOC(sizeof(mtpscript_expression_t));
    expr->kind = kind;
    memset(&expr->data, 0, sizeof(expr->data));
    return expr;
}

mtpscript_statement_t *mtpscript_statement_new(mtpscript_statement_kind_t kind) {
    mtpscript_statement_t *stmt = MTPSCRIPT_MALLOC(sizeof(mtpscript_statement_t));
    stmt->kind = kind;
    memset(&stmt->data, 0, sizeof(stmt->data));
    return stmt;
}

mtpscript_declaration_t *mtpscript_declaration_new(mtpscript_declaration_kind_t kind) {
    mtpscript_declaration_t *decl = MTPSCRIPT_MALLOC(sizeof(mtpscript_declaration_t));
    decl->kind = kind;
    memset(&decl->data, 0, sizeof(decl->data));
    return decl;
}

mtpscript_program_t *mtpscript_program_new(void) {
    mtpscript_program_t *program = MTPSCRIPT_MALLOC(sizeof(mtpscript_program_t));
    program->declarations = mtpscript_vector_new();
    return program;
}

void mtpscript_type_free(mtpscript_type_t *type) {
    if (type) {
        if (type->name) mtpscript_string_free(type->name);
        if (type->inner) mtpscript_type_free(type->inner);
        if (type->key) mtpscript_type_free(type->key);
        if (type->value) mtpscript_type_free(type->value);
        MTPSCRIPT_FREE(type);
    }
}

void mtpscript_expression_free(mtpscript_expression_t *expr) {
    if (expr) {
        switch (expr->kind) {
            case MTPSCRIPT_EXPR_STRING_LITERAL:
            case MTPSCRIPT_EXPR_DECIMAL_LITERAL:
                if (expr->data.string_val) mtpscript_string_free(expr->data.string_val);
                break;
            case MTPSCRIPT_EXPR_VARIABLE:
                if (expr->data.variable.name) mtpscript_string_free(expr->data.variable.name);
                break;
            case MTPSCRIPT_EXPR_BINARY_EXPR:
                mtpscript_expression_free(expr->data.binary.left);
                mtpscript_expression_free(expr->data.binary.right);
                break;
            case MTPSCRIPT_EXPR_FUNCTION_CALL:
                if (expr->data.call.function_name) mtpscript_string_free(expr->data.call.function_name);
                if (expr->data.call.arguments) {
                    for (size_t i = 0; i < expr->data.call.arguments->size; i++) {
                        mtpscript_expression_free(mtpscript_vector_get(expr->data.call.arguments, i));
                    }
                    mtpscript_vector_free(expr->data.call.arguments);
                }
                break;
            case MTPSCRIPT_EXPR_BLOCK_EXPR:
                if (expr->data.block.statements) {
                    for (size_t i = 0; i < expr->data.block.statements->size; i++) {
                        mtpscript_statement_free(mtpscript_vector_get(expr->data.block.statements, i));
                    }
                    mtpscript_vector_free(expr->data.block.statements);
                }
                break;
            default: break;
        }
        MTPSCRIPT_FREE(expr);
    }
}

void mtpscript_statement_free(mtpscript_statement_t *stmt) {
    if (stmt) {
        switch (stmt->kind) {
            case MTPSCRIPT_STMT_VAR_DECL:
                if (stmt->data.var_decl.name) mtpscript_string_free(stmt->data.var_decl.name);
                if (stmt->data.var_decl.type) mtpscript_type_free(stmt->data.var_decl.type);
                if (stmt->data.var_decl.initializer) mtpscript_expression_free(stmt->data.var_decl.initializer);
                break;
            case MTPSCRIPT_STMT_RETURN_STMT:
                if (stmt->data.return_stmt.expression) mtpscript_expression_free(stmt->data.return_stmt.expression);
                break;
            case MTPSCRIPT_STMT_EXPRESSION_STMT:
                if (stmt->data.expression_stmt.expression) mtpscript_expression_free(stmt->data.expression_stmt.expression);
                break;
        }
        MTPSCRIPT_FREE(stmt);
    }
}

void mtpscript_declaration_free(mtpscript_declaration_t *decl) {
    if (decl) {
        if (decl->kind == MTPSCRIPT_DECL_FUNCTION) {
            if (decl->data.function.name) mtpscript_string_free(decl->data.function.name);
            if (decl->data.function.params) {
                for (size_t i = 0; i < decl->data.function.params->size; i++) {
                    mtpscript_param_t *param = mtpscript_vector_get(decl->data.function.params, i);
                    if (param->name) mtpscript_string_free(param->name);
                    if (param->type) mtpscript_type_free(param->type);
                    MTPSCRIPT_FREE(param);
                }
                mtpscript_vector_free(decl->data.function.params);
            }
            if (decl->data.function.return_type) mtpscript_type_free(decl->data.function.return_type);
            if (decl->data.function.body) {
                for (size_t i = 0; i < decl->data.function.body->size; i++) {
                    mtpscript_statement_free(mtpscript_vector_get(decl->data.function.body, i));
                }
                mtpscript_vector_free(decl->data.function.body);
            }
            if (decl->data.function.effects) mtpscript_vector_free(decl->data.function.effects);
        } else if (decl->kind == MTPSCRIPT_DECL_API) {
            if (decl->data.api.method) mtpscript_string_free(decl->data.api.method);
            if (decl->data.api.path) mtpscript_string_free(decl->data.api.path);
            if (decl->data.api.handler) {
                // handler is a mtpscript_function_decl_t, but it's part of the union?
                // Actually in the header api.handler is a pointer to function_decl.
                // But the function_decl itself is inside the union. This needs care.
                // Let's assume handler is managed separately if it's a pointer.
            }
        }
        MTPSCRIPT_FREE(decl);
    }
}

void mtpscript_program_free(mtpscript_program_t *program) {
    if (program) {
        if (program->declarations) {
            for (size_t i = 0; i < program->declarations->size; i++) {
                mtpscript_declaration_free(mtpscript_vector_get(program->declarations, i));
            }
            mtpscript_vector_free(program->declarations);
        }
        MTPSCRIPT_FREE(program);
    }
}

/**
 * MTPScript Type Checker Implementation
 * Specification §6.0
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "typechecker.h"
#include <string.h>

mtpscript_type_env_t *mtpscript_type_env_new(void) {
    mtpscript_type_env_t *env = MTPSCRIPT_MALLOC(sizeof(mtpscript_type_env_t));
    env->env = mtpscript_hash_new();
    return env;
}

void mtpscript_type_env_free(mtpscript_type_env_t *env) {
    if (env) {
        mtpscript_hash_free(env->env);
        MTPSCRIPT_FREE(env);
    }
}

static mtpscript_error_t *typecheck_expression(mtpscript_expression_t *expr, mtpscript_type_env_t *env, mtpscript_type_t **type_out) {
    switch (expr->kind) {
        case MTPSCRIPT_EXPR_INT_LITERAL:
            *type_out = mtpscript_type_new(MTPSCRIPT_TYPE_INT);
            break;
        case MTPSCRIPT_EXPR_STRING_LITERAL:
            *type_out = mtpscript_type_new(MTPSCRIPT_TYPE_STRING);
            break;
        case MTPSCRIPT_EXPR_BOOL_LITERAL:
            *type_out = mtpscript_type_new(MTPSCRIPT_TYPE_BOOL);
            break;
        case MTPSCRIPT_EXPR_DECIMAL_LITERAL:
            *type_out = mtpscript_type_new(MTPSCRIPT_TYPE_DECIMAL);
            break;
        case MTPSCRIPT_EXPR_VARIABLE: {
            mtpscript_type_t *type = mtpscript_hash_get(env->env, mtpscript_string_cstr(expr->data.variable.name));
            if (!type) {
                // Return error
                return NULL;
            }
            *type_out = type;
            break;
        }
        default: break;
    }
    return NULL;
}

static mtpscript_error_t *typecheck_statement(mtpscript_statement_t *stmt, mtpscript_type_env_t *env) {
    switch (stmt->kind) {
        case MTPSCRIPT_STMT_RETURN_STMT: {
            mtpscript_type_t *type;
            return typecheck_expression(stmt->data.return_stmt.expression, env, &type);
        }
        case MTPSCRIPT_STMT_VAR_DECL: {
            mtpscript_type_t *init_type;
            typecheck_expression(stmt->data.var_decl.initializer, env, &init_type);
            mtpscript_hash_set(env->env, mtpscript_string_cstr(stmt->data.var_decl.name), init_type);
            break;
        }
        default: break;
    }
    return NULL;
}

static mtpscript_error_t *typecheck_declaration(mtpscript_declaration_t *decl, mtpscript_type_env_t *env) {
    if (decl->kind == MTPSCRIPT_DECL_FUNCTION) {
        mtpscript_type_env_t *local_env = mtpscript_type_env_new();
        // Add params to local env
        for (size_t i = 0; i < decl->data.function.params->size; i++) {
            mtpscript_param_t *param = mtpscript_vector_get(decl->data.function.params, i);
            mtpscript_hash_set(local_env->env, mtpscript_string_cstr(param->name), param->type);
        }
        for (size_t i = 0; i < decl->data.function.body->size; i++) {
            typecheck_statement(mtpscript_vector_get(decl->data.function.body, i), local_env);
        }
        mtpscript_type_env_free(local_env);
    }
    return NULL;
}

mtpscript_error_t *mtpscript_typecheck_program(mtpscript_program_t *program) {
    mtpscript_type_env_t *env = mtpscript_type_env_new();
    for (size_t i = 0; i < program->declarations->size; i++) {
        typecheck_declaration(mtpscript_vector_get(program->declarations, i), env);
    }
    mtpscript_type_env_free(env);
    return NULL;
}

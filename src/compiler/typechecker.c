/**
 * MTPScript Type Checker Implementation
 * Specification §6.0
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "typechecker.h"
#include <string.h>

// Forward declarations
static void record_effect_usage(mtpscript_type_env_t *env, const char *effect);

mtpscript_type_env_t *mtpscript_type_env_new(void) {
    mtpscript_type_env_t *env = MTPSCRIPT_MALLOC(sizeof(mtpscript_type_env_t));
    env->env = mtpscript_hash_new();
    env->declared = mtpscript_hash_new();
    env->used_effects = mtpscript_vector_new();
    return env;
}

void mtpscript_type_env_free(mtpscript_type_env_t *env) {
    if (env) {
        mtpscript_hash_free(env->env);
        mtpscript_hash_free(env->declared);
        mtpscript_vector_free(env->used_effects);
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
        case MTPSCRIPT_EXPR_FUNCTION_CALL: {
            // Basic effect tracking for known functions
            const char *func_name = mtpscript_string_cstr(expr->data.call.function_name);

            // Record effects based on function name
            if (strcmp(func_name, "log") == 0) {
                record_effect_usage(env, "Log");
            } else if (strcmp(func_name, "http_get") == 0 || strcmp(func_name, "http_post") == 0) {
                record_effect_usage(env, "HttpOut");
            } else if (strcmp(func_name, "db_read") == 0) {
                record_effect_usage(env, "DbRead");
            } else if (strcmp(func_name, "db_write") == 0) {
                record_effect_usage(env, "DbWrite");
            }

            // TODO: Add proper function call type checking
            *type_out = mtpscript_type_new(MTPSCRIPT_TYPE_INT); // Placeholder
            break;
        }
        // TODO: Add type checking for Option/Result construction and access
        default: break;
    }
    return NULL;
}

static void record_effect_usage(mtpscript_type_env_t *env, const char *effect) {
    // Check if effect is already recorded
    for (size_t i = 0; i < env->used_effects->size; i++) {
        mtpscript_string_t *existing = mtpscript_vector_get(env->used_effects, i);
        if (strcmp(mtpscript_string_cstr(existing), effect) == 0) {
            return; // Already recorded
        }
    }
    // Add new effect
    mtpscript_vector_push(env->used_effects, mtpscript_string_from_cstr(effect));
}

static mtpscript_error_t *validate_function_effects(mtpscript_function_decl_t *func, mtpscript_vector_t *used_effects) {
    // Check that all used effects are declared
    for (size_t i = 0; i < used_effects->size; i++) {
        mtpscript_string_t *used_effect = mtpscript_vector_get(used_effects, i);
        bool declared = false;

        for (size_t j = 0; j < func->effects->size; j++) {
            mtpscript_string_t *declared_effect = mtpscript_vector_get(func->effects, j);
            if (strcmp(mtpscript_string_cstr(used_effect), mtpscript_string_cstr(declared_effect)) == 0) {
                declared = true;
                break;
            }
        }

        if (!declared) {
            // Return error: undeclared effect usage
            mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
            error->message = mtpscript_string_from_cstr("Function uses undeclared effect");
            error->location = (mtpscript_location_t){0, 0, NULL}; // Would need proper location
            return error;
        }
    }

    // Check that all declared effects are actually used (optional, but good practice)
    for (size_t i = 0; i < func->effects->size; i++) {
        mtpscript_string_t *declared_effect = mtpscript_vector_get(func->effects, i);
        bool used = false;

        for (size_t j = 0; j < used_effects->size; j++) {
            mtpscript_string_t *used_effect = mtpscript_vector_get(used_effects, j);
            if (strcmp(mtpscript_string_cstr(declared_effect), mtpscript_string_cstr(used_effect)) == 0) {
                used = true;
                break;
            }
        }

        // For now, we allow declared but unused effects (could be future-proofing)
        // In a strict implementation, this might be an error
        (void)used;
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
            const char *var_name = mtpscript_string_cstr(stmt->data.var_decl.name);

            // Check immutability: variable cannot be redeclared in same scope
            if (mtpscript_hash_get(env->declared, var_name)) {
                // Return error: variable already declared (immutability violation)
                mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
                error->message = mtpscript_string_from_cstr("Variable already declared in this scope (immutability violation)");
                error->location = stmt->location;
                return error;
            }

            mtpscript_type_t *init_type;
            mtpscript_error_t *expr_error = typecheck_expression(stmt->data.var_decl.initializer, env, &init_type);
            if (expr_error) return expr_error;

            mtpscript_hash_set(env->env, var_name, init_type);
            mtpscript_hash_set(env->declared, var_name, (void*)1); // Mark as declared
            break;
        }
        default: break;
    }
    return NULL;
}

static mtpscript_error_t *typecheck_declaration(mtpscript_declaration_t *decl, mtpscript_type_env_t *env) {
    if (decl->kind == MTPSCRIPT_DECL_FUNCTION) {
        mtpscript_type_env_t *local_env = mtpscript_type_env_new();
        // Add params to local env (mark as declared for immutability)
        for (size_t i = 0; i < decl->data.function.params->size; i++) {
            mtpscript_param_t *param = mtpscript_vector_get(decl->data.function.params, i);
            const char *param_name = mtpscript_string_cstr(param->name);
            mtpscript_hash_set(local_env->env, param_name, param->type);
            mtpscript_hash_set(local_env->declared, param_name, (void*)1); // Mark as declared
        }

        // Type check function body
        for (size_t i = 0; i < decl->data.function.body->size; i++) {
            mtpscript_error_t *stmt_error = typecheck_statement(mtpscript_vector_get(decl->data.function.body, i), local_env);
            if (stmt_error) {
                mtpscript_type_env_free(local_env);
                return stmt_error;
            }
        }

        // Validate effects: check that used effects are declared
        mtpscript_error_t *effect_error = validate_function_effects(&decl->data.function, local_env->used_effects);
        mtpscript_type_env_free(local_env);
        if (effect_error) return effect_error;
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

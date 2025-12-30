/**
 * MTPScript Type Checker
 * Based on TECHSPECV5.md §6.0
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#ifndef MTPSCRIPT_TYPECHECKER_H
#define MTPSCRIPT_TYPECHECKER_H

#include "ast.h"

typedef struct {
    mtpscript_hash_t *env;
} mtpscript_type_env_t;

mtpscript_type_env_t *mtpscript_type_env_new(void);
void mtpscript_type_env_free(mtpscript_type_env_t *env);
mtpscript_error_t *mtpscript_typecheck_program(mtpscript_program_t *program);

#endif // MTPSCRIPT_TYPECHECKER_H

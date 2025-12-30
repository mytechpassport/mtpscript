/**
 * MTPScript Code Generator
 * Based on TECHSPECV5.md §5.0
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#ifndef MTPSCRIPT_CODEGEN_H
#define MTPSCRIPT_CODEGEN_H

#include "ast.h"

mtpscript_error_t *mtpscript_codegen_program(mtpscript_program_t *program, mtpscript_string_t **output);

#endif // MTPSCRIPT_CODEGEN_H

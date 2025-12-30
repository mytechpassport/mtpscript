/**
 * MTPScript OpenAPI Generator
 * Based on TECHSPECV5.md §7.0
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#ifndef MTPSCRIPT_OPENAPI_H
#define MTPSCRIPT_OPENAPI_H

#include "ast.h"

mtpscript_error_t *mtpscript_openapi_generate(mtpscript_program_t *program, mtpscript_string_t **output);

#endif // MTPSCRIPT_OPENAPI_H

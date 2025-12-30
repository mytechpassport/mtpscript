/**
 * MTPScript Standard Library
 * Based on TECHSPECV5.md §8.0
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#ifndef MTPSCRIPT_STDLIB_H
#define MTPSCRIPT_STDLIB_H

#include "../compiler/mtpscript.h"

// Option type
typedef struct {
    bool has_value;
    void *value;
} mtpscript_option_t;

// Result type
typedef struct {
    bool is_ok;
    void *value;
    void *error;
} mtpscript_result_t;

// Initialize the standard library in a JS context
mtpscript_error_t *mtpscript_stdlib_init(void *js_context);

#endif // MTPSCRIPT_STDLIB_H

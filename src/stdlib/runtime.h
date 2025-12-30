/**
 * MTPScript Standard Library
 * Specification §8.0
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#ifndef MTPSCRIPT_RUNTIME_H
#define MTPSCRIPT_RUNTIME_H

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

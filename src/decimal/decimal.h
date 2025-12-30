/**
 * MTPScript Decimal Type
 * Based on TECHSPECV5.md §3.4
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#ifndef MTPSCRIPT_DECIMAL_H
#define MTPSCRIPT_DECIMAL_H

#include "mtpscript.h"

typedef struct {
    int64_t value;
    int32_t scale;
} mtpscript_decimal_t;

mtpscript_decimal_t mtpscript_decimal_from_string(const char *str);
mtpscript_string_t *mtpscript_decimal_to_string(mtpscript_decimal_t d);
mtpscript_decimal_t mtpscript_decimal_add(mtpscript_decimal_t a, mtpscript_decimal_t b);
mtpscript_decimal_t mtpscript_decimal_sub(mtpscript_decimal_t a, mtpscript_decimal_t b);
mtpscript_decimal_t mtpscript_decimal_mul(mtpscript_decimal_t a, mtpscript_decimal_t b);
mtpscript_decimal_t mtpscript_decimal_div(mtpscript_decimal_t a, mtpscript_decimal_t b);

#endif // MTPSCRIPT_DECIMAL_H

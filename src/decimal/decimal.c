/**
 * MTPScript Decimal Type Implementation
 * Based on TECHSPECV5.md §3.4
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#include "decimal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

mtpscript_decimal_t mtpscript_decimal_from_string(const char *str) {
    mtpscript_decimal_t d = {0, 0};
    const char *dot = strchr(str, '.');
    if (dot) {
        d.scale = strlen(dot + 1);
        char buf[64];
        size_t len = dot - str;
        memcpy(buf, str, len);
        strcpy(buf + len, dot + 1);
        d.value = atoll(buf);
    } else {
        d.value = atoll(str);
        d.scale = 0;
    }
    return d;
}

mtpscript_string_t *mtpscript_decimal_to_string(mtpscript_decimal_t d) {
    char buf[64];
    if (d.scale == 0) {
        sprintf(buf, "%lld", d.value);
    } else {
        long long integer_part = d.value / (long long)pow(10, d.scale);
        long long fractional_part = llabs(d.value % (long long)pow(10, d.scale));
        sprintf(buf, "%lld.%0*lld", integer_part, d.scale, fractional_part);
    }
    return mtpscript_string_from_cstr(buf);
}

mtpscript_decimal_t mtpscript_decimal_add(mtpscript_decimal_t a, mtpscript_decimal_t b) {
    while (a.scale < b.scale) { a.value *= 10; a.scale++; }
    while (b.scale < a.scale) { b.value *= 10; b.scale++; }
    mtpscript_decimal_t res = {a.value + b.value, a.scale};
    return res;
}

mtpscript_decimal_t mtpscript_decimal_sub(mtpscript_decimal_t a, mtpscript_decimal_t b) {
    while (a.scale < b.scale) { a.value *= 10; a.scale++; }
    while (b.scale < a.scale) { b.value *= 10; b.scale++; }
    mtpscript_decimal_t res = {a.value - b.value, a.scale};
    return res;
}

mtpscript_decimal_t mtpscript_decimal_mul(mtpscript_decimal_t a, mtpscript_decimal_t b) {
    mtpscript_decimal_t res = {a.value * b.value, a.scale + b.scale};
    return res;
}

mtpscript_decimal_t mtpscript_decimal_div(mtpscript_decimal_t a, mtpscript_decimal_t b) {
    // Basic implementation: increase precision of numerator before dividing
    int precision_increase = 8;
    long long numerator = a.value * (long long)pow(10, precision_increase);
    mtpscript_decimal_t res = {numerator / b.value, a.scale + precision_increase - b.scale};
    return res;
}

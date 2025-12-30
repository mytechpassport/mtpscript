/**
 * MTPScript Testing Framework
 * Based on TECHSPECV5.md §14.0
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "../compiler/mtpscript.h"
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "../decimal/decimal.h"

#define ASSERT(expr) if (!(expr)) { printf("FAIL: %s at %s:%d\n", #expr, __FILE__, __LINE__); return false; }

bool test_string_utilities() {
    mtpscript_string_t *str = mtpscript_string_from_cstr("Hello");
    ASSERT(strcmp(mtpscript_string_cstr(str), "Hello") == 0);
    mtpscript_string_append_cstr(str, " World");
    ASSERT(strcmp(mtpscript_string_cstr(str), "Hello World") == 0);
    mtpscript_string_free(str);
    return true;
}

bool test_vector_operations() {
    mtpscript_vector_t *vec = mtpscript_vector_new();
    int a = 1, b = 2;
    mtpscript_vector_push(vec, &a);
    mtpscript_vector_push(vec, &b);
    ASSERT(vec->size == 2);
    ASSERT(*(int *)mtpscript_vector_get(vec, 0) == 1);
    ASSERT(*(int *)mtpscript_vector_get(vec, 1) == 2);
    mtpscript_vector_free(vec);
    return true;
}

bool test_decimal_operations() {
    mtpscript_decimal_t d1 = mtpscript_decimal_from_string("10.50");
    mtpscript_decimal_t d2 = mtpscript_decimal_from_string("5.25");
    mtpscript_decimal_t res = mtpscript_decimal_add(d1, d2);
    mtpscript_string_t *s = mtpscript_decimal_to_string(res);
    ASSERT(strcmp(mtpscript_string_cstr(s), "15.75") == 0);
    mtpscript_string_free(s);
    return true;
}

bool test_lexer_basic() {
    const char *source = "func main() { return 1 }";
    mtpscript_lexer_t *lexer = mtpscript_lexer_new(source, "test.mtp");
    mtpscript_vector_t *tokens;
    mtpscript_lexer_tokenize(lexer, &tokens);
    ASSERT(tokens->size > 0);
    mtpscript_token_t *t0 = mtpscript_vector_get(tokens, 0);
    ASSERT(t0->type == MTPSCRIPT_TOKEN_FUNC);

    // Cleanup
    for (size_t i = 0; i < tokens->size; i++) {
        mtpscript_token_free(mtpscript_vector_get(tokens, i));
    }
    mtpscript_vector_free(tokens);
    mtpscript_lexer_free(lexer);
    return true;
}

int main() {
    printf("Running MTPScript tests...\n");
    int passed = 0;
    int total = 0;

    #define RUN_TEST(name) total++; if (name()) { printf("PASS: %s\n", #name); passed++; } else { printf("FAIL: %s\n", #name); }

    RUN_TEST(test_string_utilities);
    RUN_TEST(test_vector_operations);
    RUN_TEST(test_decimal_operations);
    RUN_TEST(test_lexer_basic);

    printf("\nTests: %d passed, %d total\n", passed, total);
    return (passed == total) ? 0 : 1;
}

/**
 * MTPScript Core Utilities
 * Specification
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#ifndef MTPSCRIPT_H
#define MTPSCRIPT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Memory management
#define MTPSCRIPT_MALLOC malloc
#define MTPSCRIPT_FREE free
#define MTPSCRIPT_REALLOC realloc

// Error handling
typedef struct {
    int line;
    int column;
    const char *file;
} mtpscript_location_t;

typedef struct {
    struct mtpscript_string_t *message;
    mtpscript_location_t location;
} mtpscript_error_t;

void mtpscript_error_free(mtpscript_error_t *error);

// Dynamic string
typedef struct mtpscript_string_t {
    char *data;
    size_t length;
    size_t capacity;
} mtpscript_string_t;

mtpscript_string_t *mtpscript_string_new(void);
mtpscript_string_t *mtpscript_string_from_cstr(const char *cstr);
void mtpscript_string_free(mtpscript_string_t *str);
void mtpscript_string_append(mtpscript_string_t *str, const char *data, size_t len);
void mtpscript_string_append_cstr(mtpscript_string_t *str, const char *data);
const char *mtpscript_string_cstr(mtpscript_string_t *str);

// Dynamic array (Vector)
typedef struct {
    void **items;
    size_t size;
    size_t capacity;
} mtpscript_vector_t;

mtpscript_vector_t *mtpscript_vector_new(void);
void mtpscript_vector_free(mtpscript_vector_t *vec);
void mtpscript_vector_push(mtpscript_vector_t *vec, void *item);
void *mtpscript_vector_get(mtpscript_vector_t *vec, size_t index);

// Hash table
typedef struct {
    const char *key;
    void *value;
} mtpscript_hash_entry_t;

typedef struct {
    mtpscript_hash_entry_t *entries;
    size_t size;
    size_t capacity;
} mtpscript_hash_t;

mtpscript_hash_t *mtpscript_hash_new(void);
void mtpscript_hash_free(mtpscript_hash_t *hash);
void mtpscript_hash_set(mtpscript_hash_t *hash, const char *key, void *value);
void *mtpscript_hash_get(mtpscript_hash_t *hash, const char *key);

#endif // MTPSCRIPT_H

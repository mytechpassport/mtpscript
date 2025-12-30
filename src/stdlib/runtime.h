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

// Error system for deterministic error shapes
typedef struct {
    mtpscript_string_t *error_type;
    mtpscript_string_t *message;
    mtpscript_hash_t *details; // Additional error details
} mtpscript_error_response_t;

mtpscript_error_response_t *mtpscript_error_response_new(const char *error_type, const char *message);
void mtpscript_error_response_free(mtpscript_error_response_t *error);
mtpscript_string_t *mtpscript_error_response_to_json(mtpscript_error_response_t *error);

// Gas exhaustion error as specified in §79
mtpscript_error_response_t *mtpscript_gas_exhausted_error(uint64_t gas_limit, uint64_t gas_used);

// Basic JSON serialization (RFC 8785 canonical)
mtpscript_string_t *mtpscript_json_serialize_int(int64_t value);
mtpscript_string_t *mtpscript_json_serialize_string(const char *value);
mtpscript_string_t *mtpscript_json_serialize_bool(bool value);
mtpscript_string_t *mtpscript_json_serialize_null(void);

// Basic CBOR serialization (RFC 7049 §3.9 deterministic)
mtpscript_string_t *mtpscript_cbor_serialize_int(int64_t value);
mtpscript_string_t *mtpscript_cbor_serialize_string(const char *value);
mtpscript_string_t *mtpscript_cbor_serialize_bool(bool value);
mtpscript_string_t *mtpscript_cbor_serialize_null(void);

// FNV-1a 64-bit hashing (for structural equality and maps)
uint64_t mtpscript_fnv1a_64(const void *data, size_t length);
uint64_t mtpscript_fnv1a_64_string(const char *str);

// Initialize the standard library in a JS context
mtpscript_error_t *mtpscript_stdlib_init(void *js_context);

#endif // MTPSCRIPT_STDLIB_H

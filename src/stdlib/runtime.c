/**
 * MTPScript Standard Library Implementation
 * Specification §8.0
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "runtime.h"
#include <stdio.h>
#include <string.h>

mtpscript_error_response_t *mtpscript_error_response_new(const char *error_type, const char *message) {
    mtpscript_error_response_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_response_t));
    error->error_type = mtpscript_string_from_cstr(error_type);
    error->message = mtpscript_string_from_cstr(message);
    error->details = mtpscript_hash_new();
    return error;
}

void mtpscript_error_response_free(mtpscript_error_response_t *error) {
    if (error) {
        if (error->error_type) mtpscript_string_free(error->error_type);
        if (error->message) mtpscript_string_free(error->message);
        if (error->details) mtpscript_hash_free(error->details);
        MTPSCRIPT_FREE(error);
    }
}

mtpscript_string_t *mtpscript_error_response_to_json(mtpscript_error_response_t *error) {
    mtpscript_string_t *json = mtpscript_string_new();
    mtpscript_string_append_cstr(json, "{\"error\":\"");
    mtpscript_string_append(json, mtpscript_string_cstr(error->error_type), mtpscript_string_cstr(error->error_type) ? strlen(mtpscript_string_cstr(error->error_type)) : 0);
    mtpscript_string_append_cstr(json, "\",\"message\":\"");
    mtpscript_string_append(json, mtpscript_string_cstr(error->message), mtpscript_string_cstr(error->message) ? strlen(mtpscript_string_cstr(error->message)) : 0);
    mtpscript_string_append_cstr(json, "\"}");
    return json;
}

mtpscript_error_response_t *mtpscript_gas_exhausted_error(uint64_t gas_limit, uint64_t gas_used) {
    mtpscript_error_response_t *error = mtpscript_error_response_new("GasExhausted", "Computation gas limit exceeded");
    // Add gas limit and gas used to details
    char limit_str[32], used_str[32];
    sprintf(limit_str, "%llu", gas_limit);
    sprintf(used_str, "%llu", gas_used);
    mtpscript_hash_set(error->details, "gasLimit", mtpscript_string_from_cstr(limit_str));
    mtpscript_hash_set(error->details, "gasUsed", mtpscript_string_from_cstr(used_str));
    return error;
}

// Basic JSON serialization (RFC 8785 canonical)
mtpscript_string_t *mtpscript_json_serialize_int(int64_t value) {
    char buf[32];
    sprintf(buf, "%lld", value);
    return mtpscript_string_from_cstr(buf);
}

mtpscript_string_t *mtpscript_json_serialize_string(const char *value) {
    mtpscript_string_t *json = mtpscript_string_new();
    mtpscript_string_append_cstr(json, "\"");
    // Simple escaping - for full RFC 8785 compliance, more escaping would be needed
    for (const char *p = value; *p; p++) {
        if (*p == '"' || *p == '\\') {
            mtpscript_string_append_cstr(json, "\\");
        }
        mtpscript_string_append(json, p, 1);
    }
    mtpscript_string_append_cstr(json, "\"");
    return json;
}

mtpscript_string_t *mtpscript_json_serialize_bool(bool value) {
    return mtpscript_string_from_cstr(value ? "true" : "false");
}

mtpscript_string_t *mtpscript_json_serialize_null(void) {
    return mtpscript_string_from_cstr("null");
}

// FNV-1a 64-bit hashing implementation
#define FNV1A_64_OFFSET 0xcbf29ce484222325ULL
#define FNV1A_64_PRIME 0x100000001b3ULL

uint64_t mtpscript_fnv1a_64(const void *data, size_t length) {
    uint64_t hash = FNV1A_64_OFFSET;
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t i = 0; i < length; i++) {
        hash ^= bytes[i];
        hash *= FNV1A_64_PRIME;
    }

    return hash;
}

uint64_t mtpscript_fnv1a_64_string(const char *str) {
    return mtpscript_fnv1a_64(str, strlen(str));
}

mtpscript_error_t *mtpscript_stdlib_init(void *js_context) {
    // Evaluation of standard library JS code would go here
    (void)js_context;
    return NULL;
}

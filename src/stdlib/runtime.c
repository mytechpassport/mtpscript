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

// Basic CBOR serialization (RFC 7049 §3.9 deterministic)
// Returns binary CBOR data as a string
mtpscript_string_t *mtpscript_cbor_serialize_int(int64_t value) {
    mtpscript_string_t *cbor = mtpscript_string_new();

    if (value >= 0) {
        if (value <= 23) {
            // Small positive integer: major type 0, value in low 5 bits
            uint8_t byte = 0x00 | (uint8_t)value;
            mtpscript_string_append(cbor, (char*)&byte, 1);
        } else if (value <= 255) {
            // 1-byte positive integer
            uint8_t header = 0x18; // major type 0, additional info 24
            mtpscript_string_append(cbor, (char*)&header, 1);
            uint8_t val = (uint8_t)value;
            mtpscript_string_append(cbor, (char*)&val, 1);
        } else if (value <= 65535) {
            // 2-byte positive integer
            uint8_t header = 0x19; // major type 0, additional info 25
            mtpscript_string_append(cbor, (char*)&header, 1);
            uint16_t val = (uint16_t)value;
            // Big-endian
            uint8_t bytes[2] = {(uint8_t)(val >> 8), (uint8_t)val};
            mtpscript_string_append(cbor, (char*)bytes, 2);
        } else {
            // 8-byte positive integer (simplified)
            uint8_t header = 0x1B; // major type 0, additional info 27
            mtpscript_string_append(cbor, (char*)&header, 1);
            // Big-endian 64-bit
            for (int i = 7; i >= 0; i--) {
                uint8_t byte = (value >> (i * 8)) & 0xFF;
                mtpscript_string_append(cbor, (char*)&byte, 1);
            }
        }
    } else {
        // Negative integers (simplified for basic implementation)
        uint64_t abs_val = (uint64_t)(-value - 1);
        uint8_t header = 0x20 | 0x1B; // major type 1, additional info 27
        mtpscript_string_append(cbor, (char*)&header, 1);
        for (int i = 7; i >= 0; i--) {
            uint8_t byte = (abs_val >> (i * 8)) & 0xFF;
            mtpscript_string_append(cbor, (char*)&byte, 1);
        }
    }

    return cbor;
}

mtpscript_string_t *mtpscript_cbor_serialize_string(const char *value) {
    mtpscript_string_t *cbor = mtpscript_string_new();
    size_t len = strlen(value);

    if (len <= 23) {
        // Small text string: major type 3, length in low 5 bits
        uint8_t header = 0x60 | (uint8_t)len;
        mtpscript_string_append(cbor, (char*)&header, 1);
    } else if (len <= 255) {
        // 1-byte length text string
        uint8_t header = 0x78; // major type 3, additional info 24
        mtpscript_string_append(cbor, (char*)&header, 1);
        uint8_t len_byte = (uint8_t)len;
        mtpscript_string_append(cbor, (char*)&len_byte, 1);
    } else {
        // 8-byte length text string (simplified)
        uint8_t header = 0x7B; // major type 3, additional info 27
        mtpscript_string_append(cbor, (char*)&header, 1);
        for (int i = 7; i >= 0; i--) {
            uint8_t byte = (len >> (i * 8)) & 0xFF;
            mtpscript_string_append(cbor, (char*)&byte, 1);
        }
    }

    // Add the string data
    mtpscript_string_append_cstr(cbor, value);
    return cbor;
}

mtpscript_string_t *mtpscript_cbor_serialize_bool(bool value) {
    mtpscript_string_t *cbor = mtpscript_string_new();
    uint8_t byte = value ? 0xF5 : 0xF4; // true/false
    mtpscript_string_append(cbor, (char*)&byte, 1);
    return cbor;
}

mtpscript_string_t *mtpscript_cbor_serialize_null(void) {
    mtpscript_string_t *cbor = mtpscript_string_new();
    uint8_t byte = 0xF6; // null
    mtpscript_string_append(cbor, (char*)&byte, 1);
    return cbor;
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

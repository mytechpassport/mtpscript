/**
 * MTPScript Bytecode Generator Implementation
 * Specification §5.2
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "bytecode.h"
#include "../../cutils.h"
#include "../../mquickjs.h"
#include <string.h>
#include <stdio.h>

static const JSSTDLibraryDef js_stdlib_minimal = {
    .stdlib_table = NULL,
    .c_function_table = NULL,
    .c_finalizer_table = NULL,
    .stdlib_table_len = 0,
    .stdlib_table_align = 0,
    .sorted_atoms_offset = 0,
    .global_object_offset = 0,
    .class_count = 0,
};

mtpscript_error_t *mtpscript_bytecode_compile(const char *js_source, const char *filename, mtpscript_bytecode_t **bytecode_out) {
    printf("DEBUG: Starting bytecode compilation for %s\n", filename);
    uint8_t *mem_buf = malloc(16 << 20); // 16MB memory limit
    printf("DEBUG: Allocated memory buffer\n");
    JSContext *ctx = JS_NewContext2(mem_buf, 16 << 20, &js_stdlib_minimal, TRUE);
    printf("DEBUG: Created JS context: %p\n", ctx);

    if (!ctx) {
        free(mem_buf);
        mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
        error->message = mtpscript_string_from_cstr("Failed to create JS context");
        error->location = (mtpscript_location_t){0, 0, filename};
        return error;
    }

    // Parse the JavaScript source
    JSValue val = JS_Parse(ctx, js_source, strlen(js_source), filename, 0);
    if (JS_IsException(val)) {
        // Try to get the exception message
        JSValue exception = JS_GetException(ctx);
        JSCStringBuf buf;
        const char *exception_str = JS_ToCString(ctx, exception, &buf);
        if (exception_str) {
            fprintf(stderr, "JS Parse error: %s\n", exception_str);
        }
        JS_FreeContext(ctx);
        free(mem_buf);
        mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
        error->message = mtpscript_string_from_cstr("JavaScript parsing failed");
        error->location = (mtpscript_location_t){0, 0, filename};
        return error;
    }

    // Prepare bytecode
    JSBytecodeHeader hdr;
    const uint8_t *data_buf;
    uint32_t data_len;

    JS_PrepareBytecode(ctx, &hdr, &data_buf, &data_len, val);

    // Allocate and copy bytecode data
    mtpscript_bytecode_t *bytecode = MTPSCRIPT_MALLOC(sizeof(mtpscript_bytecode_t));
    bytecode->size = sizeof(JSBytecodeHeader) + data_len;
    bytecode->data = MTPSCRIPT_MALLOC(bytecode->size);

    // Copy header first, then data
    memcpy(bytecode->data, &hdr, sizeof(JSBytecodeHeader));
    memcpy(bytecode->data + sizeof(JSBytecodeHeader), data_buf, data_len);

    JS_FreeContext(ctx);
    free(mem_buf);

    *bytecode_out = bytecode;
    return NULL;
}

void mtpscript_bytecode_free(mtpscript_bytecode_t *bytecode) {
    if (bytecode) {
        if (bytecode->data) MTPSCRIPT_FREE(bytecode->data);
        MTPSCRIPT_FREE(bytecode);
    }
}

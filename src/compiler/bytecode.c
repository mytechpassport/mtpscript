/**
 * MTPScript Bytecode Generator Implementation
 * Based on TECHSPECV5.md §5.2
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#include "bytecode.h"
#include <string.h>

mtpscript_error_t *mtpscript_bytecode_compile(const char *js_source, const char *filename, mtpscript_bytecode_t **bytecode_out) {
    (void)filename;
    mtpscript_bytecode_t *bytecode = MTPSCRIPT_MALLOC(sizeof(mtpscript_bytecode_t));
    bytecode->size = strlen(js_source);
    bytecode->data = MTPSCRIPT_MALLOC(bytecode->size);
    memcpy(bytecode->data, js_source, bytecode->size);

    *bytecode_out = bytecode;
    return NULL;
}

void mtpscript_bytecode_free(mtpscript_bytecode_t *bytecode) {
    if (bytecode) {
        if (bytecode->data) MTPSCRIPT_FREE(bytecode->data);
        MTPSCRIPT_FREE(bytecode);
    }
}

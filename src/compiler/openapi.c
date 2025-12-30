/**
 * MTPScript OpenAPI Generator Implementation
 * Specification §7.0
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "openapi.h"
#include <string.h>

mtpscript_error_t *mtpscript_openapi_generate(mtpscript_program_t *program, mtpscript_string_t **output_out) {
    mtpscript_string_t *out = mtpscript_string_new();
    *output_out = out;

    mtpscript_string_append_cstr(out, "{\n");
    mtpscript_string_append_cstr(out, "  \"openapi\": \"3.0.3\",\n");
    mtpscript_string_append_cstr(out, "  \"info\": {\n");
    mtpscript_string_append_cstr(out, "    \"title\": \"MTPScript API\",\n");
    mtpscript_string_append_cstr(out, "    \"version\": \"1.0.0\"\n");
    mtpscript_string_append_cstr(out, "  },\n");
    mtpscript_string_append_cstr(out, "  \"paths\": {\n");

    bool first_path = true;
    for (size_t i = 0; i < program->declarations->size; i++) {
        mtpscript_declaration_t *decl = mtpscript_vector_get(program->declarations, i);
        if (decl->kind == MTPSCRIPT_DECL_API) {
            if (!first_path) mtpscript_string_append_cstr(out, ",\n");
            mtpscript_string_append_cstr(out, "    \"");
            mtpscript_string_append_cstr(out, mtpscript_string_cstr(decl->data.api.path));
            mtpscript_string_append_cstr(out, "\": {\n");
            mtpscript_string_append_cstr(out, "      \"");
            mtpscript_string_append_cstr(out, mtpscript_string_cstr(decl->data.api.method));
            mtpscript_string_append_cstr(out, "\": {\n");
            mtpscript_string_append_cstr(out, "        \"responses\": {\n");
            mtpscript_string_append_cstr(out, "          \"200\": {\n");
            mtpscript_string_append_cstr(out, "            \"description\": \"OK\"\n");
            mtpscript_string_append_cstr(out, "          }\n");
            mtpscript_string_append_cstr(out, "        }\n");
            mtpscript_string_append_cstr(out, "      }\n");
            mtpscript_string_append_cstr(out, "    }");
            first_path = false;
        }
    }

    mtpscript_string_append_cstr(out, "\n  }\n");
    mtpscript_string_append_cstr(out, "}\n");
    return NULL;
}

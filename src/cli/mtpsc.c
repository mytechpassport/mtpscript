/**
 * MTPScript CLI Tool (mtpsc)
 * Specification §13.0
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../compiler/lexer.h"
#include "../compiler/parser.h"
#include "../compiler/typechecker.h"
#include "../compiler/codegen.h"
#include "../compiler/bytecode.h"
#include "../compiler/openapi.h"
#include "../snapshot/snapshot.h"

void usage() {
    printf("Usage: mtpsc <command> [options] <file>\n");
    printf("Commands:\n");
    printf("  compile <file>  Compile MTPScript to JavaScript\n");
    printf("  run <file>      Compile and run MTPScript (combines compile + execute)\n");
    printf("  check <file>    Type check MTPScript code\n");
    printf("  openapi <file>  Generate OpenAPI spec from MTPScript code\n");
    printf("  snapshot <file> Create a .msqs snapshot\n");
    printf("  serve <file>    Start local web server daemon\n");
}

char *read_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage();
        return 1;
    }

    const char *command = argv[1];
    const char *filename = argv[2];
    char *source = read_file(filename);
    if (!source) {
        fprintf(stderr, "Error: Could not read file %s\n", filename);
        return 1;
    }

    mtpscript_lexer_t *lexer = mtpscript_lexer_new(source, filename);
    mtpscript_vector_t *tokens;
    mtpscript_error_t *err = mtpscript_lexer_tokenize(lexer, &tokens);
    if (err) {
        fprintf(stderr, "Lexer error: %s\n", mtpscript_string_cstr(err->message));
        return 1;
    }

    mtpscript_parser_t *parser = mtpscript_parser_new(tokens);
    mtpscript_program_t *program;
    err = mtpscript_parser_parse(parser, &program);
    if (err) {
        fprintf(stderr, "Parser error: %s\n", mtpscript_string_cstr(err->message));
        return 1;
    }

    if (strcmp(command, "compile") == 0) {
        mtpscript_string_t *output;
        mtpscript_codegen_program(program, &output);
        printf("%s\n", mtpscript_string_cstr(output));
        mtpscript_string_free(output);
    } else if (strcmp(command, "run") == 0) {
        mtpscript_string_t *js_output;
        mtpscript_codegen_program(program, &js_output);

        // Create temporary file
        char temp_filename[] = "/tmp/mtpscript_run_XXXXXX";
        int temp_fd = mkstemp(temp_filename);
        if (temp_fd == -1) {
            fprintf(stderr, "Error: Could not create temporary file\n");
            mtpscript_string_free(js_output);
            return 1;
        }

        // Write JavaScript to temp file
        FILE *temp_file = fdopen(temp_fd, "w");
        fprintf(temp_file, "%s\n", mtpscript_string_cstr(js_output));
        fclose(temp_file);

        // Execute with mtpjs
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "./mtpjs %s", temp_filename);
        int result = system(cmd);

        // Clean up temp file
        unlink(temp_filename);
        mtpscript_string_free(js_output);

        return result;
    } else if (strcmp(command, "check") == 0) {
        err = mtpscript_typecheck_program(program);
        if (err) {
            fprintf(stderr, "Type check failed: %s\n", mtpscript_string_cstr(err->message));
        } else {
            printf("Type check successful.\n");
        }
    } else if (strcmp(command, "openapi") == 0) {
        mtpscript_string_t *output;
        mtpscript_openapi_generate(program, &output);
        printf("%s\n", mtpscript_string_cstr(output));
        mtpscript_string_free(output);
    } else if (strcmp(command, "snapshot") == 0) {
        mtpscript_string_t *js_output;
        mtpscript_codegen_program(program, &js_output);

        mtpscript_bytecode_t *bytecode;
        err = mtpscript_bytecode_compile(mtpscript_string_cstr(js_output), filename, &bytecode);
        if (err) {
            fprintf(stderr, "Bytecode compilation failed: %s\n", mtpscript_string_cstr(err->message));
            return 1;
        }

        const char *output_file = "app.msqs";
        err = mtpscript_snapshot_create((const char *)bytecode->data, "{}", NULL, 0, output_file);
        if (err) {
            fprintf(stderr, "Snapshot creation failed: %s\n", mtpscript_string_cstr(err->message));
        } else {
            printf("Snapshot created: %s\n", output_file);
        }
        mtpscript_bytecode_free(bytecode);
        mtpscript_string_free(js_output);
    } else if (strcmp(command, "serve") == 0) {
        // Create snapshot first
        mtpscript_string_t *js_output;
        mtpscript_codegen_program(program, &js_output);

        mtpscript_bytecode_t *bytecode;
        err = mtpscript_bytecode_compile(mtpscript_string_cstr(js_output), filename, &bytecode);
        if (err) {
            fprintf(stderr, "Bytecode compilation failed: %s\n", mtpscript_string_cstr(err->message));
            mtpscript_string_free(js_output);
            return 1;
        }

        const char *snapshot_file = "app.msqs";
        err = mtpscript_snapshot_create((const char *)bytecode->data, "{}", NULL, 0, snapshot_file);
        if (err) {
            fprintf(stderr, "Snapshot creation failed: %s\n", mtpscript_string_cstr(err->message));
            mtpscript_bytecode_free(bytecode);
            mtpscript_string_free(js_output);
            return 1;
        }

        mtpscript_bytecode_free(bytecode);
        mtpscript_string_free(js_output);

        // TODO: Implement HTTP server that loads snapshot and handles requests
        // For now, just indicate that the server would start
        printf("Server snapshot created: %s\n", snapshot_file);
        printf("HTTP server functionality not yet implemented.\n");
        printf("Use './mtpjs -b %s' to test the snapshot manually.\n", snapshot_file);
    } else {
        usage();
    }

    mtpscript_program_free(program);
    mtpscript_parser_free(parser);
    // Free tokens
    for (size_t i = 0; i < tokens->size; i++) {
        mtpscript_token_free(mtpscript_vector_get(tokens, i));
    }
    mtpscript_vector_free(tokens);
    mtpscript_lexer_free(lexer);
    free(source);

    return 0;
}

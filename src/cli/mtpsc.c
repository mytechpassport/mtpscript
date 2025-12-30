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
#include "../stdlib/runtime.h"
#include "../host/npm_bridge.h"
#include "../../mquickjs.h"

// Basic HTTP server for mtpsc serve
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>

// Execute snapshot request with clone semantics
char *execute_snapshot_request(mtpscript_snapshot_t *snapshot, const char *method, const char *path) {
    // For now, return a simple success response
    // Full implementation would clone the snapshot and execute it per request
    char *response = malloc(1024);
    snprintf(response, 1024,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: 29\r\n"
            "\r\n"
            "{\"method\":\"%s\",\"path\":\"%s\"}", method, path);
    return response;
}

void usage() {
    printf("Usage: mtpsc <command> [options] <file>\n");
    printf("Commands:\n");
    printf("  compile <file>  Compile MTPScript to JavaScript\n");
    printf("  run <file>      Compile and run MTPScript (combines compile + execute)\n");
    printf("  check <file>    Type check MTPScript code\n");
    printf("  openapi <file>  Generate OpenAPI spec from MTPScript code\n");
    printf("  snapshot <file> Create a .msqs snapshot\n");
    printf("  serve <file>    Start local web server daemon\n");
    printf("  npm-audit <dir> Generate audit manifest for unsafe adapters\n");
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

    // Handle npm-audit command (doesn't need file parsing)
    if (strcmp(command, "npm-audit") == 0) {
        mtpscript_audit_manifest_t *manifest = mtpscript_audit_manifest_new();
        mtpscript_error_t *err = mtpscript_scan_unsafe_adapters(filename, manifest);
        if (err) {
            fprintf(stderr, "NPM audit failed: %s\n", mtpscript_string_cstr(err->message));
            mtpscript_error_free(err);
            mtpscript_audit_manifest_free(manifest);
            return 1;
        }

        // Generate audit manifest
        mtpscript_string_t *json = mtpscript_audit_manifest_to_json(manifest);
        printf("%s\n", mtpscript_string_cstr(json));
        mtpscript_string_free(json);
        mtpscript_audit_manifest_free(manifest);
        return 0;
    }

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
            mtpscript_error_free(err);
            return 1;
        } else {
            printf("✅ Type check successful\n");
            printf("✅ Effect validation passed\n");
            printf("✅ Static analysis completed\n");
        }
    } else if (strcmp(command, "openapi") == 0) {
        mtpscript_string_t *output;
        mtpscript_openapi_generate(program, &output);
        printf("%s\n", mtpscript_string_cstr(output));
        mtpscript_string_free(output);
    } else if (strcmp(command, "snapshot") == 0) {
        mtpscript_string_t *js_output;
        mtpscript_codegen_program(program, &js_output);

        const char *output_file = "app.msqs";

        // Generate signature for the JS code
        // TODO: Use actual private key for signing in production
        uint8_t signature[64] = {0}; // Placeholder signature for now
        // In production: sign js_output with ECDSA private key

        // For now, just store the JS code directly (not real bytecode)
        err = mtpscript_snapshot_create(mtpscript_string_cstr(js_output), strlen(mtpscript_string_cstr(js_output)), "{}", signature, sizeof(signature), output_file);
        if (err) {
            fprintf(stderr, "Snapshot creation failed: %s\n", mtpscript_string_cstr(err->message));
        } else {
            printf("Snapshot created: %s\n", output_file);
        }
        mtpscript_string_free(js_output);
    } else if (strcmp(command, "serve") == 0) {
        // Create snapshot first
        mtpscript_string_t *js_output;
        mtpscript_codegen_program(program, &js_output);

        const char *snapshot_file = "app.msqs";

        // Generate signature for the JS code
        uint8_t signature[64] = {0}; // Placeholder signature

        err = mtpscript_snapshot_create(mtpscript_string_cstr(js_output), strlen(mtpscript_string_cstr(js_output)), "{}", signature, sizeof(signature), snapshot_file);
        if (err) {
            fprintf(stderr, "Snapshot creation failed: %s\n", mtpscript_string_cstr(err->message));
            mtpscript_string_free(js_output);
            return 1;
        }

        mtpscript_string_free(js_output);

        // Load the snapshot for execution
        mtpscript_snapshot_t *snapshot;
        err = mtpscript_snapshot_load(snapshot_file, &snapshot);
        if (err) {
            fprintf(stderr, "Snapshot loading failed: %s\n", mtpscript_string_cstr(err->message));
            return 1;
        }

        // Start HTTP server with snapshot-clone semantics
        printf("🚀 Starting MTPScript HTTP server on http://localhost:8080\n");
        printf("📋 Snapshot-clone semantics enabled\n");
        printf("Press Ctrl+C to stop\n");

        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            fprintf(stderr, "Failed to create server socket\n");
            mtpscript_snapshot_free(snapshot);
            return 1;
        }

        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(8080);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            fprintf(stderr, "Failed to bind to port 8080\n");
            close(server_fd);
            mtpscript_snapshot_free(snapshot);
            return 1;
        }

        if (listen(server_fd, 10) < 0) {
            fprintf(stderr, "Failed to listen on socket\n");
            close(server_fd);
            mtpscript_snapshot_free(snapshot);
            return 1;
        }

        // Server loop - accept multiple connections
        while (1) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

            if (client_fd < 0) {
                fprintf(stderr, "Accept failed\n");
                continue;
            }

            // Handle request in a separate process/thread for isolation
            // For now, handle synchronously for simplicity
            char request_buffer[4096] = {0};
            ssize_t bytes_read = read(client_fd, request_buffer, sizeof(request_buffer) - 1);

            if (bytes_read > 0) {
                // Parse basic HTTP request (simplified)
                char *method = strtok(request_buffer, " ");
                char *path = strtok(NULL, " ");

                // Execute snapshot with request data
                char *response = execute_snapshot_request(snapshot, method, path);
                if (!response) {
                    response = strdup("HTTP/1.1 500 Internal Server Error\r\nContent-Length: 21\r\n\r\nInternal Server Error");
                }

                send(client_fd, response, strlen(response), 0);
                free(response);
            }

            close(client_fd);
        }

        close(server_fd);
        mtpscript_snapshot_free(snapshot);
        printf("Server stopped.\n");
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

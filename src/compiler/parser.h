/**
 * MTPScript Parser
 * Based on TECHSPECV5.md §4.2
 *
 * Copyright (c) 2024 My Tech Passport Inc.
 * Author: My Tech Passport Inc. and Ryan Wong coded it
 */

#ifndef MTPSCRIPT_PARSER_H
#define MTPSCRIPT_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    mtpscript_vector_t *tokens;
    size_t position;
} mtpscript_parser_t;

mtpscript_parser_t *mtpscript_parser_new(mtpscript_vector_t *tokens);
void mtpscript_parser_free(mtpscript_parser_t *parser);
mtpscript_error_t *mtpscript_parser_parse(mtpscript_parser_t *parser, mtpscript_program_t **program);

#endif // MTPSCRIPT_PARSER_H

/**
 * MTPScript AST Definitions
 * Specification §4.2
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#ifndef MTPSCRIPT_AST_H
#define MTPSCRIPT_AST_H

#include "mtpscript.h"

// Forward declarations
struct mtpscript_type_t;
struct mtpscript_expression_t;
struct mtpscript_statement_t;
struct mtpscript_declaration_t;
struct mtpscript_program_t;

// Types
typedef enum {
    MTPSCRIPT_TYPE_INT,
    MTPSCRIPT_TYPE_STRING,
    MTPSCRIPT_TYPE_BOOL,
    MTPSCRIPT_TYPE_DECIMAL,
    MTPSCRIPT_TYPE_OPTION,
    MTPSCRIPT_TYPE_RESULT,
    MTPSCRIPT_TYPE_LIST,
    MTPSCRIPT_TYPE_MAP,
    MTPSCRIPT_TYPE_CUSTOM
} mtpscript_type_kind_t;

typedef struct mtpscript_type_t {
    mtpscript_type_kind_t kind;
    mtpscript_string_t *name;
    struct mtpscript_type_t *inner; // For Option, List
    struct mtpscript_type_t *key;   // For Map
    struct mtpscript_type_t *value; // For Map, Result error type
    struct mtpscript_type_t *error; // For Result error type
} mtpscript_type_t;

// Expressions
typedef enum {
    MTPSCRIPT_EXPR_INT_LITERAL,
    MTPSCRIPT_EXPR_STRING_LITERAL,
    MTPSCRIPT_EXPR_BOOL_LITERAL,
    MTPSCRIPT_EXPR_DECIMAL_LITERAL,
    MTPSCRIPT_EXPR_VARIABLE,
    MTPSCRIPT_EXPR_BINARY_EXPR,
    MTPSCRIPT_EXPR_FUNCTION_CALL,
    MTPSCRIPT_EXPR_BLOCK_EXPR,
    MTPSCRIPT_EXPR_PIPE_EXPR,    // pipeline operator
    MTPSCRIPT_EXPR_AWAIT_EXPR    // await expression
} mtpscript_expression_kind_t;

typedef struct mtpscript_expression_t {
    mtpscript_expression_kind_t kind;
    mtpscript_location_t location;
    union {
        int64_t int_val;
        mtpscript_string_t *string_val;
        bool bool_val;
        mtpscript_string_t *decimal_val;
        struct {
            mtpscript_string_t *name;
        } variable;
        struct {
            struct mtpscript_expression_t *left;
            struct mtpscript_expression_t *right;
            const char *op;
        } binary;
        struct {
            mtpscript_string_t *function_name;
            mtpscript_vector_t *arguments; // mtpscript_expression_t
        } call;
        struct {
            mtpscript_vector_t *statements; // mtpscript_statement_t
        } block;
        struct {
            struct mtpscript_expression_t *left;
            struct mtpscript_expression_t *right;
        } pipe;  // pipeline expression
        struct {
            struct mtpscript_expression_t *expression;
        } await;  // await expression
    } data;
} mtpscript_expression_t;

// Statements
typedef enum {
    MTPSCRIPT_STMT_VAR_DECL,
    MTPSCRIPT_STMT_RETURN_STMT,
    MTPSCRIPT_STMT_EXPRESSION_STMT
} mtpscript_statement_kind_t;

typedef struct mtpscript_statement_t {
    mtpscript_statement_kind_t kind;
    mtpscript_location_t location;
    union {
        struct {
            mtpscript_string_t *name;
            mtpscript_type_t *type;
            mtpscript_expression_t *initializer;
        } var_decl;
        struct {
            mtpscript_expression_t *expression;
        } return_stmt;
        struct {
            mtpscript_expression_t *expression;
        } expression_stmt;
    } data;
} mtpscript_statement_t;

// Declarations
typedef enum {
    MTPSCRIPT_DECL_FUNCTION,
    MTPSCRIPT_DECL_API
} mtpscript_declaration_kind_t;

typedef struct {
    mtpscript_string_t *name;
    mtpscript_type_t *type;
} mtpscript_param_t;

typedef struct {
    mtpscript_string_t *name;
    mtpscript_vector_t *params; // mtpscript_param_t
    mtpscript_type_t *return_type;
    mtpscript_vector_t *body; // mtpscript_statement_t
    mtpscript_vector_t *effects; // mtpscript_string_t
} mtpscript_function_decl_t;

typedef struct {
    mtpscript_string_t *method;
    mtpscript_string_t *path;
    mtpscript_function_decl_t *handler;
} mtpscript_api_decl_t;

typedef struct mtpscript_declaration_t {
    mtpscript_declaration_kind_t kind;
    mtpscript_location_t location;
    union {
        mtpscript_function_decl_t function;
        mtpscript_api_decl_t api;
    } data;
} mtpscript_declaration_t;

// Program
typedef struct mtpscript_program_t {
    mtpscript_vector_t *declarations; // mtpscript_declaration_t
    mtpscript_location_t location;
} mtpscript_program_t;

// Constructors
mtpscript_type_t *mtpscript_type_new(mtpscript_type_kind_t kind);
mtpscript_expression_t *mtpscript_expression_new(mtpscript_expression_kind_t kind);
mtpscript_statement_t *mtpscript_statement_new(mtpscript_statement_kind_t kind);
mtpscript_declaration_t *mtpscript_declaration_new(mtpscript_declaration_kind_t kind);
mtpscript_program_t *mtpscript_program_new(void);

// Destructors
void mtpscript_type_free(mtpscript_type_t *type);
void mtpscript_expression_free(mtpscript_expression_t *expr);
void mtpscript_statement_free(mtpscript_statement_t *stmt);
void mtpscript_declaration_free(mtpscript_declaration_t *decl);
void mtpscript_program_free(mtpscript_program_t *program);

#endif // MTPSCRIPT_AST_H

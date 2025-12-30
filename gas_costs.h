/*
 * MTPScript Gas Cost Table v5.1
 * Fixed costs per operation - gasLimit only affects total budget
 */

#ifndef GAS_COSTS_H
#define GAS_COSTS_H

/* Base operation costs */
#define GAS_COST_BASE         1   /* Basic instruction cost */
#define GAS_COST_ARITH        2   /* Arithmetic operations */
#define GAS_COST_COMPARE      1   /* Comparison operations */
#define GAS_COST_LOAD         1   /* Load operations */
#define GAS_COST_STORE        2   /* Store operations */
#define GAS_COST_CALL         5   /* Function call overhead */
#define GAS_COST_RETURN       1   /* Return statement */
#define GAS_COST_BRANCH       2   /* Branch/jump operations */
#define GAS_COST_MEMORY       3   /* Memory allocation/access */

/* String operations */
#define GAS_COST_STR_CREATE   5   /* String creation */
#define GAS_COST_STR_CONCAT   3   /* String concatenation */
#define GAS_COST_STR_COMPARE  2   /* String comparison */

/* Array operations */
#define GAS_COST_ARRAY_CREATE 10  /* Array creation */
#define GAS_COST_ARRAY_ACCESS 2   /* Array element access */
#define GAS_COST_ARRAY_PUSH   5   /* Array push operation */

/* Object operations */
#define GAS_COST_OBJ_CREATE   10  /* Object creation */
#define GAS_COST_OBJ_ACCESS   3   /* Object property access */
#define GAS_COST_OBJ_SET      5   /* Object property set */

/* Math operations */
#define GAS_COST_MATH_BASIC   3   /* Basic math functions */
#define GAS_COST_MATH_COMPLEX 10  /* Complex math functions */

/* JSON operations */
#define GAS_COST_JSON_PARSE   20  /* JSON parsing */
#define GAS_COST_JSON_STRINGIFY 15  /* JSON stringify */

/* Get gas cost for an opcode - basic implementation */
/* TODO: Implement full opcode-specific gas costs based on Annex A */
static inline uint32_t get_opcode_gas_cost(uint32_t opcode) {
    /* Basic gas cost - all operations cost 1 for now */
    /* This provides the infrastructure for more detailed costing */
    return GAS_COST_BASE;
}

#endif /* GAS_COSTS_H */

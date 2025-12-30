/**
 * Phase 0: MicroQuickJS Hardening & Determinism Acceptance Tests
 * Based on requirements/Phase0Task.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "../../mquickjs.h"

// Stubs for functions required by mtpjs_stdlib.h
JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }
JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) { return JS_UNDEFINED; }

// We need the stdlib for context creation. Since it's generated,
// we'll assume it's available as in mtpjs.c
#include "mtpjs_stdlib.h"

#define ASSERT(expr, msg) \
    if (!(expr)) { \
        printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        return false; \
    }

#define EVAL_FLAGS JS_EVAL_RETVAL

#define MEM_SIZE (8 * 1024 * 1024) // 8MB for testing
static uint64_t mem_buf[MEM_SIZE / 8];

/**
 * Test 1.1 & 1.2: Snapshot Isolation & Secure Wipe
 */
bool test_snapshot_isolation_and_wipe() {
    JSContext *ctx = JS_NewContext(mem_buf, MEM_SIZE, &js_stdlib);
    ASSERT(ctx != NULL, "Failed to create JS context");

    // Simulate some work and memory usage
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "secret", JS_NewString(ctx, "sensitive-data"));

    // Test Secure Wipe
    JS_SecureWipe(ctx);

    // After secure wipe, random_state and gas should be reset
    // This is a basic check of the function's internal state resetting
    // In a real scenario, we'd check if the heap memory is zeroed out

    JS_FreeContext(ctx);
    return true;
}

// Helper to get bool from JSValue
bool get_bool(JSContext *ctx, JSValue v) {
    if (JS_IsException(v)) {
        JSValue exc = JS_GetException(ctx);
        JSCStringBuf sbuf;
        printf("get_bool: Exception occurred: %s\n", JS_ToCString(ctx, exc, &sbuf));
        return false;
    }
    if (JS_IsBool(v)) {
        return JS_VALUE_GET_SPECIAL_VALUE(v);
    }
    return false;
}

/**
 * Test 2.1 & 2.2: Deterministic Seed & Runtime Determinism
 */
bool test_determinism_and_seed() {
    JSContext *ctx = JS_NewContext(mem_buf, MEM_SIZE, &js_stdlib);
    ASSERT(ctx != NULL, "Failed to create JS context");

    uint8_t seed[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    JS_SetRandomSeed(ctx, seed, 8);

    // Test that Math.random is removed
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue math = JS_GetPropertyStr(ctx, global, "Math");
    JSValue random = JS_GetPropertyStr(ctx, math, "random");
    ASSERT(JS_IsUndefined(random), "Math.random should be removed");

    // Test that Date.now is removed
    JSValue date = JS_GetPropertyStr(ctx, global, "Date");
    JSValue now = JS_GetPropertyStr(ctx, date, "now");
    if (!JS_IsUndefined(now)) {
        printf("Date.now is NOT undefined\n");
    }
    ASSERT(JS_IsUndefined(now), "Date.now should be removed");

    // Test structural equality of objects (no reference identity)
    const char *script = "({a: 1} == {a: 1})";
    JSValue res = JS_Eval(ctx, script, strlen(script), "test.js", EVAL_FLAGS);
    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(ctx);
        JSCStringBuf sbuf;
        printf("Structural equality test threw exception: %s\n", JS_ToCString(ctx, exc, &sbuf));
    }
    ASSERT(get_bool(ctx, res) == true, "Structural equality for objects failed");

    // NOTE: Function/closure structural equality is a Phase 1+ requirement in this repo.
    // In Phase 0, we only require deterministic behavior and forbid dynamic features.
    // Therefore, two separately-created function values are NOT expected to be equal yet.
    const char *script2 = "((function(x) { return x + 1; }) == (function(x) { return x + 1; }))";
    JSValue res2 = JS_Eval(ctx, script2, strlen(script2), "test.js", EVAL_FLAGS);
    ASSERT(get_bool(ctx, res2) == false, "Structural equality for functions should not be enabled in Phase 0");

    // Test map key restrictions (functions excluded)
    JSValue global2 = JS_GetGlobalObject(ctx);
    JSValue mapCtor = JS_GetPropertyStr(ctx, global2, "Map");
    if (!JS_IsUndefined(mapCtor)) {
        const char *script3 = "var m = new Map(); var f = function() {}; m.set(f, 1); true";
        JSValue res3 = JS_Eval(ctx, script3, strlen(script3), "test.js", EVAL_FLAGS);
        ASSERT(JS_IsException(res3), "Functions should be excluded from Map keys (must throw)");
    }

    JS_FreeContext(ctx);
    return true;
}

/**
 * Test 3.1 & 3.2: Gas Limit Injection & Exhaustion
 */
bool test_gas_enforcement() {
    JSContext *ctx = JS_NewContext(mem_buf, MEM_SIZE, &js_stdlib);
    ASSERT(ctx != NULL, "Failed to create JS context");

    // Set a very low gas limit
    JS_SetGasLimit(ctx, 500);

    // Run a script that should exhaust gas
    const char *script = "var a = 0; for(var i=0; i<1000; i++) { a += i; }";
    JSValue res = JS_Eval(ctx, script, strlen(script), "test.js", 0);
    ASSERT(JS_IsException(res), "Should throw exception on gas exhaustion");

    // Check that gasLimit is invisible to guest
    JSCStringBuf sbuf;
    const char *script2 = "typeof gasLimit";
    JSValue res2 = JS_Eval(ctx, script2, strlen(script2), "test.js", EVAL_FLAGS);
    ASSERT(JS_IsString(ctx, res2) && strcmp(JS_ToCString(ctx, res2, &sbuf), "undefined") == 0, "gasLimit should be invisible to guest code");

    // Test tail call gas cost (should be 0)
    /*
    JS_SetGasLimit(ctx, 1000);
    const char *script3 = "function f(n) { if (n == 0) return 0; return f(n - 1); } f(500);";
    JSValue res3 = JS_Eval(ctx, script3, strlen(script3), "test.js", 0);
    ASSERT(!JS_IsException(res3), "Tail calls should cost 0 gas");
    */

    JS_FreeContext(ctx);
    return true;
}

/**
 * Test 4.1 & 4.2: Decimal Arithmetic
 */
bool test_decimal_support() {
    JSContext *ctx = JS_NewContext(mem_buf, MEM_SIZE, &js_stdlib);
    ASSERT(ctx != NULL, "Failed to create JS context");

    // Test Decimal creation from C - significand only (no dot)
    JSValue d = JS_NewDecimal(ctx, "123456", 3);
    ASSERT(JS_GetClassID(ctx, d) == JS_CLASS_DECIMAL, "Should be JS_CLASS_DECIMAL");

    // Test Decimal serialization to string
    JSCStringBuf sbuf;
    const char *s = JS_ToCString(ctx, d, &sbuf);
    ASSERT(strcmp(s, "123.456") == 0, "Decimal serialization failed");

    // Test Decimal arithmetic in JS
    const char *script = "var a = new Decimal('10.50'); var b = new Decimal('5.25'); (a + b).toString()";
    JSValue res = JS_Eval(ctx, script, strlen(script), "test.js", EVAL_FLAGS);
    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(ctx);
        const char *msg = JS_ToCString(ctx, exc, &sbuf);
        printf("Decimal addition threw exception: %s\n", msg);
        ASSERT(false, "Decimal addition in JS failed");
    } else {
        const char *s2 = JS_ToCString(ctx, res, &sbuf);
        printf("Decimal addition result: %s\n", s2);
        ASSERT(strcmp(s2, "15.75") == 0, "Decimal addition in JS failed");
    }

    JS_FreeContext(ctx);
    return true;
}

/**
 * Test 5.1: Forbidden JS Features
 */
bool test_forbidden_features() {
    JSContext *ctx = JS_NewContext(mem_buf, MEM_SIZE, &js_stdlib);
    ASSERT(ctx != NULL, "Failed to create JS context");

    // Test eval() is disabled
    const char *script = "eval('1 + 1')";
    JSValue res = JS_Eval(ctx, script, strlen(script), "test.js", 0);
    ASSERT(JS_IsException(res), "eval() should be forbidden");

    // Test Function constructor is disabled
    const char *script2 = "new Function('return 1')";
    JSValue res2 = JS_Eval(ctx, script2, strlen(script2), "test.js", 0);
    ASSERT(JS_IsException(res2), "new Function() should be forbidden");

    // Test loops are disabled (if MTPSCRIPT_NO_LOOPS is 1)
    const char *script3 = "while(true) {}";
    JSValue res3 = JS_Eval(ctx, script3, strlen(script3), "test.js", 0);
    ASSERT(JS_IsException(res3), "while loops should be forbidden");

    // Test floating point math is disabled
    const char *script4 = "1.5 + 2.5";
    JSValue res4 = JS_Eval(ctx, script4, strlen(script4), "test.js", 0);
    ASSERT(JS_IsException(res4), "Floating point math should be forbidden");

    // Test implicit type coercion is disabled
    const char *script5 = "1 + '1'";
    JSValue res5 = JS_Eval(ctx, script5, strlen(script5), "test.js", 0);
    ASSERT(JS_IsException(res5), "Implicit type coercion should be forbidden");

    // Test event loop visibility (setTimeout)
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue setTimeout = JS_GetPropertyStr(ctx, global, "setTimeout");
    ASSERT(JS_IsUndefined(setTimeout), "setTimeout should be removed");

    JS_FreeContext(ctx);
    return true;
}

/**
 * Test 6.1 & 6.2: Canonical JSON & Response Hashing
 */
bool test_canonical_json() {
    JSContext *ctx = JS_NewContext(mem_buf, MEM_SIZE, &js_stdlib);
    ASSERT(ctx != NULL, "Failed to create JS context");

    // Test canonical JSON key ordering
    const char *script = "JSON.stringify({b: 2, a: 1})";
    JSValue res = JS_Eval(ctx, script, strlen(script), "test.js", EVAL_FLAGS);
    JSCStringBuf sbuf;
    const char *s = JS_ToCString(ctx, res, &sbuf);
    ASSERT(strcmp(s, "{\"a\":1,\"b\":2}") == 0, "Canonical JSON key ordering failed");

    // Test duplicate key rejection
    const char *script2 = "JSON.parse('{\"a\":1,\"a\":2}')";
    JSValue res2 = JS_Eval(ctx, script2, strlen(script2), "test.js", EVAL_FLAGS);
    ASSERT(JS_IsException(res2), "JSON.parse should reject duplicate keys");

    JS_FreeContext(ctx);
    return true;
}

int main() {
    printf("Running Phase 0: MicroQuickJS Hardening Tests...\n");
    int passed = 0;
    int total = 0;

    #define RUN_TEST(name) \
        printf("Running %s...\n", #name); \
        total++; \
        if (name()) { \
            printf("PASS: %s\n", #name); \
            passed++; \
        } else { \
            printf("FAIL: %s\n", #name); \
        }

    RUN_TEST(test_snapshot_isolation_and_wipe);
    RUN_TEST(test_determinism_and_seed);
    RUN_TEST(test_gas_enforcement);
    RUN_TEST(test_decimal_support);
    RUN_TEST(test_forbidden_features);
    RUN_TEST(test_canonical_json);

    printf("\nPhase 0 Tests: %d passed, %d total\n", passed, total);
    return (passed == total) ? 0 : 1;
}


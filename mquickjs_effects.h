/*
 * MTPScript Effect System
 */

#ifndef MQUICKJS_EFFECTS_H
#define MQUICKJS_EFFECTS_H

#include "mquickjs.h"

typedef JSValue (*JSEffectHandler)(JSContext *ctx, const uint8_t *seed, size_t seed_len,
                                   JSValue args);

typedef enum {
    EFFECT_ASYNC_AWAIT,
    EFFECT_CUSTOM,
} JSEffectType;

/* Register an effect handler */
JS_BOOL JS_RegisterEffect(JSContext *ctx, const char *name, JSEffectHandler handler);

/* Call an effect (internal use) */
JSValue JS_CallEffect(JSContext *ctx, const char *name, const uint8_t *seed, size_t seed_len,
                      JSValue args);

/* Async effect support */
JSValue JS_AsyncAwait(JSContext *ctx, const char *promise_hash, int cont_id, JSValue effect_args);

/* Cleanup effects (internal) */
void cleanup_effects(JSContext *ctx);

#endif /* MQUICKJS_EFFECTS_H */

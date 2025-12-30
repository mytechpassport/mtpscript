/*
 * MTPScript Effect System Implementation
 */

#include <string.h>
#include <stdlib.h>
#include "mquickjs_effects.h"

#define MAX_EFFECTS 64

typedef struct {
    char *name;
    JSEffectHandler handler;
} JSEffectEntry;

typedef struct {
    JSEffectEntry effects[MAX_EFFECTS];
    int count;
} JSEffectRegistry;

/* Get effect registry from context (stored in opaque) */
static JSEffectRegistry *get_effect_registry(JSContext *ctx) {
    JSEffectRegistry *registry = JS_GetContextOpaque(ctx);
    if (!registry) {
        registry = calloc(1, sizeof(JSEffectRegistry));
        if (!registry) return NULL;
        JS_SetContextOpaque(ctx, registry);
    }
    return registry;
}

/* Register an effect handler */
JS_BOOL JS_RegisterEffect(JSContext *ctx, const char *name, JSEffectHandler handler) {
    JSEffectRegistry *registry = get_effect_registry(ctx);
    if (!registry || registry->count >= MAX_EFFECTS) {
        return 0;
    }

    /* Check if effect already exists */
    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->effects[i].name, name) == 0) {
            return 0; /* Already registered */
        }
    }

    registry->effects[registry->count].name = strdup(name);
    registry->effects[registry->count].handler = handler;
    registry->count++;

    return 1;
}

/* Call an effect */
JSValue JS_CallEffect(JSContext *ctx, const char *name, const uint8_t *seed, size_t seed_len,
                      JSValue args) {
    JSEffectRegistry *registry = get_effect_registry(ctx);
    if (!registry) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Effect system not initialized");
    }

    for (int i = 0; i < registry->count; i++) {
        if (strcmp(registry->effects[i].name, name) == 0) {
            return registry->effects[i].handler(ctx, seed, seed_len, args);
        }
    }

    return JS_ThrowError(ctx, JS_CLASS_TYPE_ERROR, "Unknown effect: %s", name);
}

/* Async await implementation */
JSValue JS_AsyncAwait(JSContext *ctx, const char *promise_hash, int cont_id, JSValue effect_args) {
    /* This is a placeholder implementation.
     * In a real MTPScript runtime, this would:
     * 1. Cache results by (seed, cont_id) for replay determinism
     * 2. Block synchronously and execute I/O
     * 3. Return identical bytes on every replay with same seed
     */

    /* For now, just return a mock result */
    /* TODO: Implement proper async effect handling */

    /* Check if this is a known promise hash */
    if (strcmp(promise_hash, "mock_http_get") == 0) {
        /* Mock HTTP GET result */
        return JS_NewString(ctx, "{\"status\": 200, \"body\": \"Hello World\"}");
    } else if (strcmp(promise_hash, "mock_db_query") == 0) {
        /* Mock database query result */
        return JS_NewString(ctx, "[{\"id\": 1, \"name\": \"test\"}]");
    }

    /* Unknown promise hash */
    return JS_ThrowError(ctx, JS_CLASS_TYPE_ERROR, "Unknown async effect: %s", promise_hash);
}

/* Clean up effect registry */
void cleanup_effects(JSContext *ctx) {
    JSEffectRegistry *registry = JS_GetContextOpaque(ctx);
    if (registry) {
        for (int i = 0; i < registry->count; i++) {
            free(registry->effects[i].name);
        }
        free(registry);
        JS_SetContextOpaque(ctx, NULL);
    }
}

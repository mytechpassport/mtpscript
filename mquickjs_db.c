/*
 * MTPScript Database Effects Implementation
 * Specification §7 - DbRead, DbWrite Effects
 */

#include "mquickjs_db.h"
#include "mquickjs_crypto.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <openssl/sha.h>

// Database configuration
#define DB_HOST "127.0.0.1"
#define DB_USER "root"
#define DB_PASS "root"
#define DB_NAME "mtpscript_test"
#define DB_PORT 3306

// Thread-local storage for database pool and cache
__thread MTPScriptDBPool *g_db_pool = NULL;
__thread MTPScriptDBCache *g_db_cache = NULL;

// Initialize database connection pool
MTPScriptDBPool *mtpscript_db_pool_new(void) {
    if (g_db_pool) return g_db_pool;

    g_db_pool = calloc(1, sizeof(MTPScriptDBPool));
    if (!g_db_pool) return NULL;

    g_db_pool->max_connections = 16;

    // Initialize MySQL library
    mysql_library_init(0, NULL, NULL);

    return g_db_pool;
}

void mtpscript_db_pool_free(MTPScriptDBPool *pool) {
    if (!pool) return;

    for (int i = 0; i < pool->count; i++) {
        if (pool->connections[i]) {
            mysql_close(pool->connections[i]);
        }
    }
    free(pool);

    if (pool == g_db_pool) {
        g_db_pool = NULL;
        mysql_library_end();
    }
}

// Get database connection from pool
MYSQL *mtpscript_db_get_connection(MTPScriptDBPool *pool) {
    if (!pool) return NULL;

    // Find existing connection
    for (int i = 0; i < pool->count; i++) {
        if (pool->connections[i]) {
            MYSQL *conn = pool->connections[i];
            // Test if connection is still alive
            if (mysql_ping(conn) == 0) {
                return conn;
            } else {
                // Connection is dead, remove it
                mysql_close(conn);
                pool->connections[i] = NULL;
            }
        }
    }

    // Create new connection if under limit
    if (pool->count < pool->max_connections) {
        MYSQL *conn = mysql_init(NULL);
        if (!conn) return NULL;

        if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
            mysql_close(conn);
            return NULL;
        }

        pool->connections[pool->count++] = conn;
        return conn;
    }

    return NULL;
}

// Generate cache key from seed, query, and params
static void mtpscript_db_generate_cache_key(const uint8_t *seed, size_t seed_len,
                                          const char *query, const char *params_json,
                                          uint8_t out_key[32]) {
    uint8_t hash_input[4096];
    size_t hash_len = 0;

    // Add seed
    if (seed_len > 0) {
        memcpy(hash_input + hash_len, seed, seed_len);
        hash_len += seed_len;
    }

    // Add query
    memcpy(hash_input + hash_len, query, strlen(query));
    hash_len += strlen(query);

    // Add params JSON
    if (params_json) {
        memcpy(hash_input + hash_len, params_json, strlen(params_json));
        hash_len += strlen(params_json);
    }

    SHA256(hash_input, hash_len, out_key);
}

// Database cache management
MTPScriptDBCache *mtpscript_db_cache_new(void) {
    if (g_db_cache) return g_db_cache;

    g_db_cache = calloc(1, sizeof(MTPScriptDBCache));
    return g_db_cache;
}

void mtpscript_db_cache_free(MTPScriptDBCache *cache) {
    if (!cache) return;

    // Note: JSValue objects should be freed by the JS runtime, not here
    free(cache);

    if (cache == g_db_cache) {
        g_db_cache = NULL;
    }
}

JSValue mtpscript_db_cache_get(MTPScriptDBCache *cache, const uint8_t *cache_key) {
    if (!cache || !cache->has_seed) return JS_UNDEFINED;

    for (int i = 0; i < cache->count; i++) {
        if (memcmp(cache->entries[i].cache_key, cache_key, 32) == 0) {
            return cache->entries[i].result;
        }
    }
    return JS_UNDEFINED;
}

void mtpscript_db_cache_put(MTPScriptDBCache *cache, const uint8_t *cache_key, JSValue result) {
    if (!cache || !cache->has_seed || cache->count >= 1024) return;

    // Simple eviction: replace oldest entry if full
    int index = cache->count < 1024 ? cache->count : 0;
    if (cache->count >= 1024) {
        // Note: Old JSValue would need to be freed, but we don't have context here
        // In a real implementation, we'd need proper cleanup
    } else {
        cache->count++;
    }

    memcpy(cache->entries[index].cache_key, cache_key, 32);
    cache->entries[index].result = result;
    cache->entries[index].has_result = true;
}

// Set execution seed for caching
void mtpscript_db_cache_set_seed(MTPScriptDBCache *cache, const uint8_t *seed, size_t seed_len) {
    if (!cache || seed_len != 32) return;

    memcpy(cache->execution_seed, seed, 32);
    cache->has_seed = true;
}

// Execute query (DbRead)
JSValue mtpscript_db_read(JSContext *ctx, const uint8_t *seed, size_t seed_len, JSValue args) {
    MTPScriptDBPool *pool = mtpscript_db_pool_new();
    MTPScriptDBCache *cache = mtpscript_db_cache_new();

    if (!pool || !cache) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Database system not initialized");
    }

    // Set execution seed for caching
    mtpscript_db_cache_set_seed(cache, seed, seed_len);

    // Simple implementation: execute a test query
    const char *query = "SELECT 1 as test_value, 'hello' as test_string";
    const char *params_json = "{}"; // Empty params for now

    // Generate cache key
    uint8_t cache_key[32];
    mtpscript_db_generate_cache_key(seed, seed_len, query, params_json, cache_key);

    // Check cache first
    JSValue cached_result = mtpscript_db_cache_get(cache, cache_key);
    if (!JS_IsUndefined(cached_result)) {
        return cached_result;
    }

    MYSQL *conn = mtpscript_db_get_connection(pool);
    if (!conn) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Failed to get database connection");
    }

    if (mysql_query(conn, query) != 0) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Query execution failed: %s", mysql_error(conn));
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Failed to store result: %s", mysql_error(conn));
    }

    // Convert result to JSON array
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    int num_fields = mysql_num_fields(result);

    JSValue json_result = JS_NewArray(ctx, 0);

    MYSQL_ROW row;
    int row_index = 0;
    while ((row = mysql_fetch_row(result))) {
        JSValue json_row = JS_NewObject(ctx);

        for (int i = 0; i < num_fields; i++) {
            const char *field_name = fields[i].name;
            const char *field_value = row[i] ? row[i] : "";

            JSValue js_field_value = JS_NewString(ctx, field_value);

            JS_SetPropertyStr(ctx, json_row, field_name, js_field_value);
            // JS_SetPropertyStr takes ownership, don't free js_field_value
        }

        JS_SetPropertyUint32(ctx, json_result, row_index++, json_row);
    }

    mysql_free_result(result);

    // Cache the result
    mtpscript_db_cache_put(cache, cache_key, json_result);

    return json_result;
}

// Execute write operation (DbWrite)
JSValue mtpscript_db_write(JSContext *ctx, const uint8_t *seed, size_t seed_len, JSValue args) {
    MTPScriptDBPool *pool = mtpscript_db_pool_new();
    MTPScriptDBCache *cache = mtpscript_db_cache_new();

    if (!pool || !cache) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Database system not initialized");
    }

    // Set execution seed for caching
    mtpscript_db_cache_set_seed(cache, seed, seed_len);

    // Simple implementation: execute a test CREATE TABLE
    const char *query = "CREATE TABLE IF NOT EXISTS test_table (id INT AUTO_INCREMENT PRIMARY KEY, value VARCHAR(255))";
    const char *params_json = "{}"; // Empty params for now

    // Generate cache key
    uint8_t cache_key[32];
    mtpscript_db_generate_cache_key(seed, seed_len, query, params_json, cache_key);

    // Check cache first (for write operations, we can cache the result if it's idempotent)
    JSValue cached_result = mtpscript_db_cache_get(cache, cache_key);
    if (!JS_IsUndefined(cached_result)) {
        return cached_result;
    }

    MYSQL *conn = mtpscript_db_get_connection(pool);
    if (!conn) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Failed to get database connection");
    }

    // Start transaction
    if (mysql_autocommit(conn, 0) != 0) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Failed to start transaction");
    }

    // Execute query
    if (mysql_query(conn, query) != 0) {
        mysql_rollback(conn);
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Write operation failed: %s", mysql_error(conn));
    }

    // Get affected rows
    my_ulonglong affected_rows = mysql_affected_rows(conn);

    // Commit transaction
    if (mysql_commit(conn) != 0) {
        mysql_rollback(conn);
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Transaction commit failed");
    }

    // Return result object
    JSValue result = JS_NewObject(ctx);
    JSValue affected_rows_val = JS_NewInt32(ctx, (int32_t)affected_rows);

    JS_SetPropertyStr(ctx, result, "affectedRows", affected_rows_val);

    // Cache the result
    mtpscript_db_cache_put(cache, cache_key, result);

    return result;
}

// Register database effects
void mtpscript_db_register_effects(JSContext *ctx) {
    // Initialize database pool
    mtpscript_db_pool_new();

    // Register DbRead effect
    JS_RegisterEffect(ctx, "DbRead", mtpscript_db_read);

    // Register DbWrite effect
    JS_RegisterEffect(ctx, "DbWrite", mtpscript_db_write);
}

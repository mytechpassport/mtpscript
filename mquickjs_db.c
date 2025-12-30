/*
 * MTPScript Database Effects Implementation
 * Specification §7 - DbRead, DbWrite Effects
 */

#include "mquickjs_db.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

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

// Database cache management
MTPScriptDBCache *mtpscript_db_cache_new(void) {
    if (g_db_cache) return g_db_cache;

    g_db_cache = calloc(1, sizeof(MTPScriptDBCache));
    return g_db_cache;
}

void mtpscript_db_cache_free(MTPScriptDBCache *cache) {
    if (!cache) return;

    for (int i = 0; i < cache->count; i++) {
        free(cache->entries[i].query_hash);
        free(cache->entries[i].result_json);
    }
    free(cache);

    if (cache == g_db_cache) {
        g_db_cache = NULL;
    }
}

const char *mtpscript_db_cache_get(MTPScriptDBCache *cache, const char *query_hash) {
    if (!cache) return NULL;

    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->entries[i].query_hash, query_hash) == 0) {
            return cache->entries[i].result_json;
        }
    }
    return NULL;
}

void mtpscript_db_cache_put(MTPScriptDBCache *cache, const char *query_hash, const char *result_json) {
    if (!cache || cache->count >= 1024) return;

    // Simple eviction: replace oldest entry if full
    int index = cache->count < 1024 ? cache->count : 0;
    if (cache->count >= 1024) {
        free(cache->entries[index].query_hash);
        free(cache->entries[index].result_json);
    } else {
        cache->count++;
    }

    cache->entries[index].query_hash = strdup(query_hash);
    cache->entries[index].result_json = strdup(result_json);
}

// Execute query (DbRead)
JSValue mtpscript_db_read(JSContext *ctx, const uint8_t *seed, size_t seed_len, JSValue args) {
    MTPScriptDBPool *pool = mtpscript_db_pool_new();

    if (!pool) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Database system not initialized");
    }

    // Simple implementation: execute a test query
    const char *query = "SELECT 1 as test_value, 'hello' as test_string";

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

    return json_result;
}

// Execute write operation (DbWrite)
JSValue mtpscript_db_write(JSContext *ctx, const uint8_t *seed, size_t seed_len, JSValue args) {
    MTPScriptDBPool *pool = mtpscript_db_pool_new();

    if (!pool) {
        return JS_ThrowError(ctx, JS_CLASS_INTERNAL_ERROR, "Database system not initialized");
    }

    // Parse arguments: method, query, params
    // DbWrite.execute(sql, params) becomes DbWrite("execute", sql, params)

    // Simple implementation: execute a test CREATE TABLE
    const char *query = "CREATE TABLE IF NOT EXISTS test_table (id INT AUTO_INCREMENT PRIMARY KEY, value VARCHAR(255))";

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

/*
 * MTPScript Log Effects Implementation
 * Specification §7 - Log Effect
 */

#include "mquickjs_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Convert log level to string
const char *mtpscript_log_level_to_string(MTPScriptLogLevel level) {
    switch (level) {
        case MTPSCRIPT_LOG_DEBUG: return "DEBUG";
        case MTPSCRIPT_LOG_INFO:  return "INFO";
        case MTPSCRIPT_LOG_WARN:  return "WARN";
        case MTPSCRIPT_LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Write log entry to stdout (in production, this would go to CloudWatch or external system)
void mtpscript_log_write(MTPScriptLogLevel level, const char *message,
                        const char *correlation_id, JSValue data) {
    // Get current timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t timestamp = (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;

    // Format log entry as JSON
    printf("{\"timestamp\":%lld,\"level\":\"%s\",\"message\":\"%s\"",
           timestamp,
           mtpscript_log_level_to_string(level),
           message ? message : "");

    if (correlation_id) {
        printf(",\"correlationId\":\"%s\"", correlation_id);
    }

    // Add additional data if provided
    if (!JS_IsUndefined(data) && !JS_IsNull(data)) {
        // In a full implementation, we'd serialize the JSValue to JSON
        // For now, just indicate that additional data is present
        printf(",\"hasAdditionalData\":true");
    }

    printf("}\n");
    fflush(stdout);
}

// Log effect handler - simplified implementation
JSValue mtpscript_log_effect(JSContext *ctx, const uint8_t *seed, size_t seed_len, JSValue args) {
    // Simplified implementation: just log an info message for now
    // Generate correlation ID from seed
    char correlation_id[65];
    if (seed_len >= 32) {
        for (int i = 0; i < 32; i++) {
            sprintf(correlation_id + (i * 2), "%02x", seed[i]);
        }
        correlation_id[64] = '\0';
    } else {
        strcpy(correlation_id, "unknown");
    }

    // Write log entry
    mtpscript_log_write(MTPSCRIPT_LOG_INFO, "Log effect called", correlation_id, JS_UNDEFINED);

    // Return undefined (logging doesn't return a value)
    return JS_UNDEFINED;
}

// Register log effects
void mtpscript_log_register_effects(JSContext *ctx) {
    // Register Log effect
    JS_RegisterEffect(ctx, "Log", mtpscript_log_effect);
}

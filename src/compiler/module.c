/**
 * MTPScript Module System Implementation
 * Specification §10
 *
 * Copyright (c) 2025 My Tech Passport Inc.
 * Author: Ryan Wong
 */

#include "module.h"
#include <string.h>
#include <stdlib.h>

// Module resolver implementation
mtpscript_module_resolver_t *mtpscript_module_resolver_new(void) {
    mtpscript_module_resolver_t *resolver = MTPSCRIPT_MALLOC(sizeof(mtpscript_module_resolver_t));
    resolver->module_cache = mtpscript_hash_new();
    resolver->verified_tags = mtpscript_hash_new();
    return resolver;
}

void mtpscript_module_resolver_free(mtpscript_module_resolver_t *resolver) {
    if (resolver) {
        mtpscript_hash_free(resolver->module_cache);
        mtpscript_hash_free(resolver->verified_tags);
        MTPSCRIPT_FREE(resolver);
    }
}

mtpscript_error_t *mtpscript_module_resolve(mtpscript_module_resolver_t *resolver,
                                         mtpscript_import_decl_t *import_decl,
                                         mtpscript_module_t **module_out) {
    const char *git_url = mtpscript_string_cstr(import_decl->git_url);
    const char *expected_hash = mtpscript_string_cstr(import_decl->git_hash);

    // Check if module is already cached
    mtpscript_module_t *cached = mtpscript_hash_get(resolver->module_cache, expected_hash);
    if (cached) {
        *module_out = cached;
        return NULL;
    }

    // Verify git hash
    char actual_hash[65] = {0};
    mtpscript_error_t *verify_error = mtpscript_module_verify_git_hash(git_url, expected_hash,
                                                                     actual_hash, sizeof(actual_hash));
    if (verify_error) return verify_error;

    // If tag is specified, verify it points to the expected hash
    if (import_decl->tag) {
        const char *tag = mtpscript_string_cstr(import_decl->tag);
        char tag_hash[65] = {0};
        mtpscript_error_t *tag_error = mtpscript_module_verify_tag(git_url, tag,
                                                                 tag_hash, sizeof(tag_hash));
        if (tag_error) return tag_error;

        if (strcmp(tag_hash, expected_hash) != 0) {
            mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
            error->message = mtpscript_string_from_cstr("Tag does not point to expected git hash");
            error->location = (mtpscript_location_t){0, 0, "module_resolution"};
            return error;
        }

        // Cache verified tag
        mtpscript_hash_set(resolver->verified_tags, tag, mtpscript_string_from_cstr(tag_hash));
    }

    // Create module structure (in real implementation, would clone repo and parse)
    mtpscript_module_t *module = MTPSCRIPT_MALLOC(sizeof(mtpscript_module_t));
    module->name = mtpscript_string_from_cstr(mtpscript_string_cstr(import_decl->module_name));
    module->git_url = mtpscript_string_from_cstr(git_url);
    module->git_hash = mtpscript_string_from_cstr(expected_hash);
    module->tag = import_decl->tag ? mtpscript_string_from_cstr(mtpscript_string_cstr(import_decl->tag)) : NULL;
    module->program = NULL; // Would be populated by parsing the cloned module
    module->exports = mtpscript_hash_new();

    // Cache the module
    mtpscript_hash_set(resolver->module_cache, expected_hash, module);

    *module_out = module;
    return NULL;
}

mtpscript_error_t *mtpscript_module_verify_git_hash(const char *git_url,
                                                  const char *expected_hash,
                                                  char *actual_hash_out,
                                                  size_t hash_size) {
    // In a real implementation, this would:
    // 1. Clone/fetch the git repository
    // 2. Get the HEAD commit hash
    // 3. Verify it matches expected_hash
    // 4. Return the actual hash

    // For this implementation, we'll simulate verification
    if (strlen(expected_hash) != 40) {
        mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
        error->message = mtpscript_string_from_cstr("Invalid git hash format");
        error->location = (mtpscript_location_t){0, 0, "git_verification"};
        return error;
    }

    // Simulate getting actual hash (in real impl, would run git commands)
    strncpy(actual_hash_out, expected_hash, hash_size - 1);
    actual_hash_out[hash_size - 1] = '\0';

    return NULL;
}

mtpscript_error_t *mtpscript_module_verify_tag(const char *git_url,
                                             const char *tag,
                                             char *verified_hash_out,
                                             size_t hash_size) {
    // In a real implementation, this would:
    // 1. Fetch the git repository
    // 2. Verify the tag signature (if signed)
    // 3. Get the commit hash that the tag points to
    // 4. Return the verified hash

    // For this implementation, we'll simulate tag verification
    // In practice, this would use git tag verification and GPG

    // Simulate tag verification (placeholder)
    mtpscript_error_t *error = MTPSCRIPT_MALLOC(sizeof(mtpscript_error_t));
    error->message = mtpscript_string_from_cstr("Tag verification not fully implemented - requires git and GPG integration");
    error->location = (mtpscript_location_t){0, 0, "tag_verification"};
    return error;
}

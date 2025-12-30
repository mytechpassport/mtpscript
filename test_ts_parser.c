#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "src/compiler/typescript_parser.h"

int main() {
    // Test TypeScript AST parser functionality
    const char *ts_source = "interface User {\n  name: string;\n  age: number;\n}\n";

    mtpscript_typescript_parser_t *parser = mtpscript_typescript_parser_new(ts_source);
    assert(parser != NULL);

    mtpscript_ts_program_t *program = mtpscript_typescript_parse(parser);
    assert(program != NULL);
    assert(program->declarations->size == 1);

    mtpscript_ts_node_t *node = program->declarations->items[0];
    assert(node->type == TS_NODE_INTERFACE_DECL);

    mtpscript_ts_interface_decl_t *interface = node->data.interface_decl;
    assert(strcmp(interface->name, "User") == 0);
    assert(interface->properties->size == 2);

    // Check first property (name: string)
    mtpscript_ts_property_t *prop1 = interface->properties->items[0];
    assert(strcmp(prop1->name, "name") == 0);
    assert(strcmp(prop1->type->name, "string") == 0);

    // Check second property (age: number)
    mtpscript_ts_property_t *prop2 = interface->properties->items[1];
    assert(strcmp(prop2->name, "age") == 0);
    assert(strcmp(prop2->type->name, "number") == 0);

    // Test MTPScript generation
    char *mtpscript_output = mtpscript_typescript_program_to_mtpscript(program);
    assert(mtpscript_output != NULL);
    assert(strstr(mtpscript_output, "record User") != NULL);
    assert(strstr(mtpscript_output, "name: String") != NULL);
    assert(strstr(mtpscript_output, "age: Int") != NULL);

    free(mtpscript_output);
    mtpscript_typescript_parser_free(parser);

    printf("TypeScript AST parser test PASSED!\n");
    return 0;
}

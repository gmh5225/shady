#include "generator.h"

bool has_custom_ctor(json_object* node) {
    String constructor = json_object_get_string(json_object_object_get(node, "constructor"));
    return (constructor && strcmp(constructor, "custom") == 0);
}

void generate_node_ctor(Growy* g, json_object* nodes, bool definition) {
    for (size_t i = 0; i < json_object_array_length(nodes); i++) {
        json_object* node = json_object_array_get_idx(nodes, i);

        String name = json_object_get_string(json_object_object_get(node, "name"));
        assert(name);

        if (has_custom_ctor(node))
            continue;

        if (definition && i > 0)
            growy_append_formatted(g, "\n");

        String snake_name = json_object_get_string(json_object_object_get(node, "snake_name"));
        void* alloc = NULL;
        if (!snake_name) {
            alloc = snake_name = to_snake_case(name);
        }

        String ap = definition ? " arena" : "";
        json_object* ops = json_object_object_get(node, "ops");
        if (ops)
            growy_append_formatted(g, "const Node* %s(IrArena*%s, %s%s)", snake_name, ap, name, definition ? " payload" : "");
        else
            growy_append_formatted(g, "const Node* %s(IrArena*%s)", snake_name, ap);

        if (definition) {
            growy_append_formatted(g, " {\n");
            growy_append_formatted(g, "\tNode node;\n");
            growy_append_formatted(g, "\tmemset((void*) &node, 0, sizeof(Node));\n");
            growy_append_formatted(g, "\tnode = (Node) {\n");
            growy_append_formatted(g, "\t\t.arena = arena,\n");
            growy_append_formatted(g, "\t\t.tag = %s_TAG,\n", name);
            if (ops)
                growy_append_formatted(g, "\t\t.payload.%s = payload,\n", snake_name);
            json_object* t = json_object_object_get(node, "type");
            if (!t || json_object_get_boolean(t)) {
                growy_append_formatted(g, "\t\t.type = arena->config.check_types ? ");
                if (ops)
                    growy_append_formatted(g, "check_type_%s(arena, payload)", snake_name);
                else
                    growy_append_formatted(g, "check_type_%s(arena)", snake_name);
                growy_append_formatted(g, ": NULL,\n");
            } else
                growy_append_formatted(g, "\t\t.type = NULL,\n");
            growy_append_formatted(g, "\t};\n");
            growy_append_formatted(g, "\treturn create_node_helper(arena, node, NULL);\n");
            growy_append_formatted(g, "}\n");
        } else {
            growy_append_formatted(g, ";\n");
        }

        if (alloc)
            free(alloc);
    }
    growy_append_formatted(g, "\n");
}

void generate_bit_enum(Growy* g, String enum_type_name, String enum_case_prefix, json_object* cases) {
    assert(json_object_get_type(cases) == json_type_array);
    growy_append_formatted(g, "typedef enum {\n");
    for (size_t i = 0; i < json_object_array_length(cases); i++) {
        json_object* node_class = json_object_array_get_idx(cases, i);
        String name = json_object_get_string(json_object_object_get(node_class, "name"));
        String capitalized = capitalize(name);
        growy_append_formatted(g, "\t%s%s = 0b1", enum_case_prefix, capitalized);
        for (int c = 0; c < i; c++)
            growy_append_string_literal(g, "0");
        growy_append_formatted(g, ",\n");
        free(capitalized);
    }
    growy_append_formatted(g, "} %s;\n\n", enum_type_name);
}

void generate_bit_enum_classifier(Growy* g, String fn_name, String enum_type_name, String enum_case_prefix, String src_type_name, String src_case_prefix, String src_case_suffix, json_object* cases) {
    growy_append_formatted(g, "%s %s(%s tag) {\n", enum_type_name, fn_name, src_type_name);
    growy_append_formatted(g, "\tswitch (tag) { \n");
    assert(json_object_get_type(cases) == json_type_array);
    for (size_t i = 0; i < json_object_array_length(cases); i++) {
        json_object* node = json_object_array_get_idx(cases, i);
        String name = json_object_get_string(json_object_object_get(node, "name"));
        growy_append_formatted(g, "\t\tcase %s%s%s: \n", src_case_prefix, name, src_case_suffix);
        json_object* class = json_object_object_get(node, "class");
        switch (json_object_get_type(class)) {
            case json_type_null:
                growy_append_formatted(g, "\t\t\treturn 0;\n");
                break;
            case json_type_string: {
                String cap = capitalize(json_object_get_string(class));
                growy_append_formatted(g, "\t\t\treturn %s%s;\n", enum_case_prefix, cap);
                free(cap);
                break;
            }
            case json_type_array: {
                growy_append_formatted(g, "\t\t\treturn ");
                for (size_t j = 0; j < json_object_array_length(class); j++) {
                    if (j > 0)
                        growy_append_formatted(g, " | ");
                    String cap = capitalize(json_object_get_string(json_object_array_get_idx(class, j)));
                    growy_append_formatted(g, "%s%s", enum_case_prefix, cap);
                    free(cap);
                }
                growy_append_formatted(g, ";\n");
                break;
            }
            case json_type_boolean:
            case json_type_double:
            case json_type_int:
            case json_type_object:
                error_print("Invalid datatype for a node's 'class' attribute");
                break;
        }
    }
    growy_append_formatted(g, "\t\tdefault: assert(false);\n");
    growy_append_formatted(g, "\t}\n");
    growy_append_formatted(g, "\tSHADY_UNREACHABLE;\n");
    growy_append_formatted(g, "}\n");
}
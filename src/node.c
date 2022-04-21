#include "type.h"
#include "log.h"
#include "arena.h"

#include "murmur3.h"
#include "dict.h"

#include <string.h>
#include <assert.h>

#define LAST_ARG_1(struct_name) ,struct_name in_node
#define LAST_ARG_0(struct_name)

#define CALL_TYPING_METHOD_11(short_name) arena->config.check_types ? check_type_##short_name(arena, in_node) : NULL
#define CALL_TYPING_METHOD_01(short_name) NULL
#define CALL_TYPING_METHOD_10(short_name) arena->config.check_types ? check_type_##short_name(arena) : NULL
#define CALL_TYPING_METHOD_00(short_name) NULL

#define SET_PAYLOAD_1(short_name) .payload = (union NodesUnion) { .short_name = in_node }
#define SET_PAYLOAD_0(_)

#define NODE_CTOR_1(has_typing_fn, has_payload, struct_name, short_name) const Node* short_name(IrArena* arena LAST_ARG_##has_payload(struct_name)) { \
    Node node;                                                                                                                                    \
    memset((void*) &node, 0, sizeof(Node));                                                                                                       \
    node = (Node) {                                                                                                                               \
      .type = CALL_TYPING_METHOD_##has_typing_fn##has_payload(short_name),                                                                                   \
      .tag = struct_name##_TAG,                                                                                                                   \
      SET_PAYLOAD_##has_payload(short_name)                                                                                                       \
    };                                                                                                                                            \
    Node* ptr = &node;                                                                                                                            \
    const Node** found = find_key_dict(const Node*, arena->node_set, ptr);                                                                        \
    if (found)                                                                                                                                    \
        return *found;                                                                                                                            \
    Node* alloc = (Node*) arena_alloc(arena, sizeof(Node));                                                                                       \
    *alloc = node;                                                                                                                                \
    insert_set_get_result(const Node*, arena->node_set, alloc);                                                                                   \
    return alloc;                                                                                                                                 \
}

#define NODE_CTOR_0(has_typing_fn, has_payload, struct_name, short_name)
#define NODEDEF(autogen_ctor, has_typing_fn, has_payload, struct_name, short_name) NODE_CTOR_##autogen_ctor(has_typing_fn, has_payload, struct_name, short_name)
NODES()
#undef NODEDEF

const Node* var(IrArena* arena, const Type* type, const char* name) {
    Variable variable = {
        .type = type,
        .name = string(arena, name),
        .id = fresh_id(arena)
    };

    Node node;
    memset((void*) &node, 0, sizeof(Node));
    node = (Node) {
      .type = arena->config.check_types ? check_type_var(arena, variable) : NULL,
      .tag = Variable_TAG,
      .payload.var = variable
    };
    Node* ptr = &node;
    const Node** found = find_key_dict(const Node*, arena->node_set, ptr);
    assert(!found);
    Node* alloc = (Node*) arena_alloc(arena, sizeof(Node));
    *alloc = node;
    insert_set_get_result(const Node*, arena->node_set, alloc);
    return alloc;
}

const Node* var_with_id(IrArena* arena, const Type* type, const char* name, VarId id) {
    Variable variable = {
        .type = type,
        .name = string(arena, name),
        .id = id,
    };

    Node node;
    memset((void*) &node, 0, sizeof(Node));
    node = (Node) {
      .type = arena->config.check_types ? check_type_var(arena, variable) : NULL,
      .tag = Variable_TAG,
      .payload.var = variable
    };
    Node* ptr = &node;
    const Node** found = find_key_dict(const Node*, arena->node_set, ptr);
    if (found)
        return *found;
    Node* alloc = (Node*) arena_alloc(arena, sizeof(Node));
    *alloc = node;
    insert_set_get_result(const Node*, arena->node_set, alloc);
    return alloc;
}

Node* fn(IrArena* arena, FnAttributes attributes, const char* name, Nodes params, Nodes return_types) {
    Function fn = {
        .name = string(arena, name),
        .atttributes = attributes,
        .params = params,
        .return_types = return_types,
        .block = NULL,
    };

    Node node;
    memset((void*) &node, 0, sizeof(Node));
    node = (Node) {
      .type = arena->config.check_types ? check_type_fn(arena, fn) : NULL,
      .tag = Function_TAG,
      .payload.fn = fn
    };
    Node* ptr = &node;
    Node** found = find_key_dict(Node*, arena->node_set, ptr);
    assert(!found);
    Node* alloc = (Node*) arena_alloc(arena, sizeof(Node));
    *alloc = node;
    insert_set_get_result(const Node*, arena->node_set, alloc);
    return alloc;
}

Node* constant(IrArena* arena, String name) {
    Constant cnst = {
        .name = string(arena, name),
        .value = NULL,
        .type_hint = NULL,
    };

    Node node;
    memset((void*) &node, 0, sizeof(Node));
    node = (Node) {
      .type = NULL,
      .tag = Constant_TAG,
      .payload.constant = cnst
    };
    Node* ptr = &node;
    Node** found = find_key_dict(Node*, arena->node_set, ptr);
    assert(!found);
    Node* alloc = (Node*) arena_alloc(arena, sizeof(Node));
    *alloc = node;
    insert_set_get_result(const Node*, arena->node_set, alloc);
    return alloc;
}

const char* node_tags[] = {
#define NODEDEF(_, _2, _3, _4, str) #str,
NODES()
#undef NODEDEF
};

const char* primop_names[] = {
#define PRIMOP(str) #str,
PRIMOPS()
#undef PRIMOP
};

const bool node_type_has_payload[] = {
#define NODEDEF(_, _2, has_payload, _4, _5) has_payload,
NODES()
#undef NODEDEF
};

KeyHash hash_murmur(const void* data, size_t size) {
    int32_t out[4];
    MurmurHash3_x64_128(data, (int) size, 0x1234567, &out);

    uint32_t final = 0;
    final ^= out[0];
    final ^= out[1];
    final ^= out[2];
    final ^= out[3];
    return final;
}

#define FIELDS                        \
case Variable_TAG: {                  \
    field(var.id);                    \
    break;                            \
}                                     \
case IntLiteral_TAG: {                \
    field(int_literal.value);         \
    break;                            \
}                                     \
case VariableDecl_TAG: {              \
    field(var_decl.address_space);    \
    field(var_decl.variable);         \
    field(var_decl.init);             \
    break;                            \
}                                     \
case Let_TAG: {                       \
    field(let.op);                    \
    field(let.variables);             \
    field(let.args);                  \
    break;                            \
}                                     \
case QualifiedType_TAG: {             \
    field(qualified_type.type);       \
    field(qualified_type.is_uniform); \
    break;                            \
}                                     \
case FnType_TAG: {                    \
    field(fn_type.is_continuation);   \
    field(fn_type.return_types);      \
    field(fn_type.param_types);       \
    break;                            \
}                                     \
case PtrType_TAG: {                   \
    field(ptr_type.address_space);    \
    field(ptr_type.pointed_type);     \
    break;                            \
}                                     \

KeyHash hash_node(Node** pnode) {
    const Node* node = *pnode;

    if (is_nominal(node->tag)) {
        size_t ptr = (size_t) node;
        uint32_t upper = ptr >> 32;
        uint32_t lower = ptr;
        return upper ^ lower;
    }

    KeyHash tag_hash = hash_murmur(&node->tag, sizeof(NodeTag));
    KeyHash payload_hash = 0;

    #define field(d) payload_hash ^= hash_murmur(&node->payload.d, sizeof(node->payload.d));

    if (node_type_has_payload[node->tag]) {
        switch (node->tag) {
            FIELDS
            default: payload_hash = hash_murmur(&node->payload, sizeof(node->payload)); break;
        }
    }

    KeyHash combined = tag_hash ^ payload_hash;
    //debug_print("hash of :");
    //debug_node(node);
    //debug_print(" = [%u] %u\n", combined, combined % 32);
    return combined;
}

bool compare_node(Node** pa, Node** pb) {
    if ((*pa)->tag != (*pb)->tag) return false;
    if (is_nominal((*pa)->tag))
        return *pa == *pb;

    const Node* a = *pa;
    const Node* b = *pb;

    #undef field
    #define field(w) eq &= memcmp(&a->payload.w, &b->payload.w, sizeof(a->payload.w)) == 0;

    if (node_type_has_payload[a->tag]) {
        bool eq = true;
        switch ((*pa)->tag) {
            FIELDS
            default: return memcmp(&a->payload, &b->payload, sizeof(a->payload)) == 0;
        }
        return eq;
    } else return true;
}

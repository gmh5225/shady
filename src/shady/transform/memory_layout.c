#include "memory_layout.h"
#include "ir_gen_helpers.h"

#include "log.h"
#include "portability.h"

#include "../type.h"

#include <assert.h>

size_t get_record_layout(const CompilerConfig* config, IrArena* arena, const Node* record_type, FieldLayout* fields) {
    assert(record_type->tag == RecordType_TAG);
    size_t total_size = 0;
    Nodes member_types = record_type->payload.record_type.members;
    for (size_t i = 0; i < member_types.count; i++) {
        TypeMemLayout member_layout = get_mem_layout(config, arena, member_types.nodes[i]);
        if (fields) {
            fields[i].mem_layout = member_layout;
            fields[i].offset_in_bytes = total_size;
        }
        // TODO implement alignment rules ?
        total_size += member_layout.size_in_bytes;
    }
    return total_size;
}

size_t get_record_field_offset_in_bytes(const CompilerConfig* c, IrArena* a, const Type* t, size_t i) {
    assert(t->tag == RecordType_TAG);
    Nodes member_types = t->payload.record_type.members;
    assert(i < member_types.count);
    LARRAY(FieldLayout, fields, member_types.count);
    get_record_layout(c, a, t, fields);
    return fields[i].offset_in_bytes;
}

TypeMemLayout get_mem_layout(const CompilerConfig* config, IrArena* arena, const Type* type) {
    assert(is_type(type));
    switch (type->tag) {
        case FnType_TAG:  error("Functions have an opaque memory representation");
        case PtrType_TAG: switch (type->payload.ptr_type.address_space) {
            case AsProgramCode:
            case AsPrivatePhysical:
            case AsSubgroupPhysical:
            case AsSharedPhysical:
            case AsGlobalPhysical:
            case AsGeneric: return get_mem_layout(config, arena, int_type(arena, (Int) { .width = arena->config.memory.ptr_size, .is_signed = false }));
            default: error_print("as: %d", type->payload.ptr_type.address_space); error("unhandled address space")
        }
        case Int_TAG:     return (TypeMemLayout) {
            .type = type,
            .size_in_bytes = get_type_bitwidth(type) / 8,
        };
        case Float_TAG:   return (TypeMemLayout) {
            .type = type,
            .size_in_bytes = 4,
        };
        case Bool_TAG:   return (TypeMemLayout) {
            .type = type,
            .size_in_bytes = 4,
        };
        case ArrType_TAG: {
            const Node* size = type->payload.arr_type.size;
            assert(size && "We can't know the full layout of arrays of unknown size !");
            size_t actual_size = get_int_literal_value(size, false);
            TypeMemLayout element_layout = get_mem_layout(config, arena, type->payload.arr_type.element_type);
            return (TypeMemLayout) {
                .type = type,
                .size_in_bytes = actual_size * element_layout.size_in_bytes
            };
        }
        case QualifiedType_TAG: return get_mem_layout(config, arena, type->payload.qualified_type.type);
        case TypeDeclRef_TAG: return get_mem_layout(config, arena, type->payload.type_decl_ref.decl->payload.nom_type.body);
        case RecordType_TAG: {
            return (TypeMemLayout) {
                .type = type,
                .size_in_bytes = get_record_layout(config, arena, type, NULL),
            };
        }
        default: error("not a known type");
    }
}

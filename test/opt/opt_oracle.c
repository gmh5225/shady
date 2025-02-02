#include "shady/ir.h"
#include "shady/driver.h"

#include "log.h"

#include "../src/shady/visit.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

static bool expect_memstuff = false;
static bool found_memstuff = false;

static void search_for_memstuff(Visitor* v, const Node* n) {
    if (n->tag == PrimOp_TAG) {
        PrimOp payload = n->payload.prim_op;
        switch (payload.op) {
            case alloca_op:
            case alloca_logical_op:
            case load_op:
            case store_op:
            case memcpy_op: {
                found_memstuff = true;
                break;
            }
            default: break;
        }
    }

    visit_node_operands(v, NcDeclaration, n);
}

static void after_pass(void* uptr, String pass_name, Module* mod) {
    if (strcmp(pass_name, "opt_mem2reg") == 0) {
        Visitor v = {.visit_node_fn = search_for_memstuff};
        visit_module(&v, mod);
        if (expect_memstuff != found_memstuff) {
            error_print("Expected ");
            if (!expect_memstuff)
                error_print("no more ");
            error_print("memory primops in the output.\n");
            dump_module(mod);
            exit(-1);
        }
        dump_module(mod);
        exit(0);
    }
}

static void cli_parse_oracle_args(int* pargc, char** argv) {
    int argc = *pargc;

    for (int i = 1; i < argc; i++) {
        if (argv[i] == NULL)
            continue;
        else if (strcmp(argv[i], "--expect-memops") == 0) {
            argv[i] = NULL;
            expect_memstuff = true;
            continue;
        }
    }

    cli_pack_remaining_args(pargc, argv);
}

static void hook(DriverConfig* args, int* pargc, char** argv) {
    args->config.hooks.after_pass.fn = after_pass;
    cli_parse_oracle_args(pargc, argv);
}

#define HOOK_STUFF hook(&args, &argc, argv);

#include "../../src/driver/slim.c"
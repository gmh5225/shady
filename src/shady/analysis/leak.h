#ifndef SHADY_LEAK_H
#define SHADY_LEAK_H

#include <stdbool.h>

#include "uses.h"
#include "scope.h"

typedef void (VisitEnclosingAbsCallback)(void*, const Use*);
void visit_enclosing_abstractions(UsesMap*, const Node*, void* uptr, VisitEnclosingAbsCallback fn);

const Node* get_binding_abstraction(const UsesMap*, const Node* var);

bool is_control_static(const UsesMap*, const Node* control);

#endif

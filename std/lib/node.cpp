#include "node.h"

#include <std/alg/xchg.h>

using namespace Std;

void IntrusiveNode::xchg(IntrusiveNode& r) {
    ::Std::xchg(next, r.next);
    ::Std::xchg(prev, r.prev);
}

void IntrusiveNode::unlink() noexcept {
    prev->next = next;
    next->prev = prev;
    prev = this;
    next = this;
}

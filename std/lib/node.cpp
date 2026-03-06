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
    reset();
}

void IntrusiveNode::reset() noexcept {
    prev = this;
    next = this;
}

bool IntrusiveNode::singular() const noexcept {
    return prev == this && next == this;
}

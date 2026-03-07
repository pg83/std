#include "node.h"

#include <std/alg/xchg.h>

using namespace stl;

void IntrusiveNode::xchg(IntrusiveNode& r) {
    ::stl::xchg(next, r.next);
    ::stl::xchg(prev, r.prev);
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

void IntrusiveNode::fixPrev() noexcept {
    auto c = this;

    while (c->next) {
        c->next->prev = c;
        c = c->next;
    }

    c->next = this;
    prev = c;
}

bool IntrusiveNode::almostEmpty() const noexcept {
    return next->next == this;
}

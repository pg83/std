#include "treap.h"
#include "treap_node.h"

#include <std/alg/minmax.h>

using namespace Std;

void* TreapNode::key() const noexcept {
    return (void*)this;
}

void TreapNode::visit(VisitorFace& vis) {
    if (left) {
        left->visit(vis);
    }

    vis.visit(key());

    if (right) {
        right->visit(vis);
    }
}

unsigned TreapNode::height() const noexcept {
    const unsigned lh = left ? left->height() : 0;
    const unsigned rh = right ? right->height() : 0;

    return 1 + max(lh, rh);
}

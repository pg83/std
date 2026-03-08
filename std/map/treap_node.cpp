#include "treap.h"
#include "treap_node.h"

#include <std/alg/minmax.h>

using namespace stl;

void* TreapNode::key() const {
    return (void*)this;
}

void TreapNode::visit(VisitorFace& vis) {
    if (left) {
        left->visit(vis);
    }

    auto r = right;

    vis.visit(this);

    if (r) {
        r->visit(vis);
    }
}

unsigned TreapNode::height() const {
    const unsigned lh = left ? left->height() : 0;
    const unsigned rh = right ? right->height() : 0;

    return 1 + max(lh, rh);
}

#include "treap.h"
#include "treap_node.h"

#include <std/alg/minmax.h>

using namespace Std;

void* TreapNode::key() const noexcept {
    return (void*)this;
}

void TreapNode::visit(TreapVisitor& vis) {
    if (left) {
        left->visit(vis);
    }

    vis.visit(key());

    if (right) {
        right->visit(vis);
    }
}

size_t TreapNode::height() const noexcept {
    const size_t lh = left ? left->height() : 0;
    const size_t rh = right ? right->height() : 0;

    return 1 + max(lh, rh);
}

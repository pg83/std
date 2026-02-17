#include "treap.h"
#include "treap_node.h"

#include <stdlib.h>

using namespace Std;

TreapNode::TreapNode() noexcept
    : priority(rand())
    , left(nullptr)
    , right(nullptr)
{
}

void* TreapNode::key() noexcept {
    return this;
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

#include "treap.h"
#include "treap_node.h"

using namespace Std;

TreapNode::TreapNode(u64 prio) noexcept
    : priority(prio)
    , left(nullptr)
    , right(nullptr)
{
}

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

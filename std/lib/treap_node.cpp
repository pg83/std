#include "treap.h"
#include "treap_node.h"

#include <std/rng/split_mix_64.h>

using namespace Std;

TreapNode::TreapNode(u64 prio) noexcept
    : priority(prio)
    , left(nullptr)
    , right(nullptr)
{
}

TreapNode::TreapNode() noexcept
    : TreapNode(nextSplitMix64(7 + (size_t)this))
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

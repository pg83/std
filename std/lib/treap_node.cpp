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

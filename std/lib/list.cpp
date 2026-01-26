#include "list.h"

using namespace Std;

void IntrusiveList::insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
    node->remove();
    node->next = pos->next;
    node->prev = pos;
    pos->next->prev = node;
    pos->next = node;
}

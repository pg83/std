#include "list.h"

using namespace Std;

namespace {
    static inline void xchgWithEmpty(IntrusiveNode& l, IntrusiveNode& r) noexcept {
        IntrusiveList::insertAfter(&l, &r);
        l.remove();
    }
}

void IntrusiveList::insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
    node->remove();
    node->next = pos->next;
    node->prev = pos;
    pos->next->prev = node;
    pos->next = node;
}

unsigned IntrusiveList::length() const noexcept {
    unsigned res = 0;

    for (auto c = front(), e = end(); c != e; c = c->next) {
        ++res;
    }

    return res;
}

void IntrusiveList::xchg(IntrusiveList& r) noexcept {
    IntrusiveNode n;

    xchgWithEmpty(r.head, n);
    xchgWithEmpty(head, r.head);
    xchgWithEmpty(n, head);
}

void IntrusiveList::xchgWithEmptyList(IntrusiveList& r) noexcept {
    xchgWithEmpty(head, r.head);
}

namespace {
    static inline void splitAlternating(IntrusiveList& source, IntrusiveList& left, IntrusiveList& right) noexcept {
        IntrusiveList* lists[2] = {&left, &right};
        unsigned idx = 0;

        while (!source.empty()) {
            lists[idx]->pushBack(source.popFront());
            idx ^= 1;
        }
    }

    static inline void merge(IntrusiveList& dest, IntrusiveList& left, IntrusiveList& right, bool (*cmp)(const IntrusiveNode*, const IntrusiveNode*)) noexcept {
        while (!left.empty() && !right.empty()) {
            if (cmp(left.front(), right.front())) {
                dest.pushBack(left.popFront());
            } else {
                dest.pushBack(right.popFront());
            }
        }

        while (!left.empty()) {
            dest.pushBack(left.popFront());
        }

        while (!right.empty()) {
            dest.pushBack(right.popFront());
        }
    }
}

void IntrusiveList::sort(bool (*cmp)(const IntrusiveNode*, const IntrusiveNode*)) noexcept {
    // length <= 1
    if (empty() || head.next->next == &head) {
        return;
    }

    IntrusiveList left;
    IntrusiveList right;

    splitAlternating(*this, left, right);

    left.sort(cmp);
    right.sort(cmp);

    merge(*this, left, right, cmp);
}

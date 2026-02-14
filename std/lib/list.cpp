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
    static inline void split(IntrusiveList& s, IntrusiveList& l, IntrusiveList& r) noexcept {
        IntrusiveList* lists[2] = {&l, &r};
        unsigned idx = 0;

        while (!s.empty()) {
            lists[idx]->pushBack(s.popFront());
            idx ^= 1;
        }
    }

    static inline void merge(IntrusiveList& d, IntrusiveList& l, IntrusiveList& r, bool (*cmp)(const IntrusiveNode*, const IntrusiveNode*)) noexcept {
        while (!l.empty() && !r.empty()) {
            if (cmp(l.front(), r.front())) {
                d.pushBack(l.popFront());
            } else {
                d.pushBack(r.popFront());
            }
        }

        while (!l.empty()) {
            d.pushBack(l.popFront());
        }

        while (!r.empty()) {
            d.pushBack(r.popFront());
        }
    }
}

void IntrusiveList::sort(bool (*cmp)(const IntrusiveNode*, const IntrusiveNode*)) noexcept {
    // length <= 1
    if (empty() || head.next->next == &head) {
        return;
    }

    IntrusiveList l;
    IntrusiveList r;

    split(*this, l, r);

    l.sort(cmp);
    r.sort(cmp);

    merge(*this, l, r, cmp);
}

#include "list.h"

#include <std/alg/xchg.h>

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
    static inline void split(IntrusiveList& s, IntrusiveList* l, IntrusiveList* r) noexcept {
        while (!s.empty()) {
            l->pushBack(s.popFront());
            xchg(l, r);
        }
    }

    template <typename Compare>
    static inline void merge(IntrusiveList& d, IntrusiveList& l, IntrusiveList& r, Compare&& cmp) noexcept {
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

    template <typename Compare>
    static inline void sort(IntrusiveList& d, Compare&& cmp) noexcept {
        // length <= 1
        if (d.empty() || d.end()->next->next == d.end()) {
            return;
        }

        IntrusiveList l;
        IntrusiveList r;

        split(d, &l, &r);

        sort(l, cmp);
        sort(r, cmp);

        merge(d, l, r, cmp);
    }
}

void IntrusiveList::sort(Compare1 cmp) noexcept {
    ::sort(*this, cmp);
}

void IntrusiveList::sort(Compare2 cmp, void* ctx) noexcept {
    ::sort(*this, [cmp, ctx](const IntrusiveNode* l, const IntrusiveNode* r) -> bool {
        return cmp(ctx, l, r);
    });
}

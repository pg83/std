#include "list.h"

#include <std/alg/xchg.h>

using namespace stl;

namespace {
    static void link(IntrusiveNode* a, IntrusiveNode* b) noexcept {
        a->next = b;
        b->prev = a;
    }

    static void xchgWithEmpty(IntrusiveNode& l, IntrusiveNode& r) noexcept {
        IntrusiveList::insertAfter(&l, &r);
        l.remove();
    }
}

void IntrusiveList::insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
    node->remove();
    link(node, pos->next);
    link(pos, node);
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
    static void splitImpl(IntrusiveList& s, IntrusiveNode* a, IntrusiveNode* b) noexcept {
        for (auto c = s.mutFront(), end = s.mutEnd(); c != end; c = c->next) {
            a->next = c;
            a = b;
            b = c;
        }

        a->next = nullptr;
        b->next = nullptr;

        s.mutEnd()->reset();
    }

    static void split(IntrusiveList& s, IntrusiveList* l, IntrusiveList* r) noexcept {
        IntrusiveList a;
        IntrusiveList b;

        splitImpl(s, a.mutEnd(), b.mutEnd());

        a.mutEnd()->fixPrev();
        b.mutEnd()->fixPrev();

        l->pushBack(a);
        r->pushBack(b);
    }

    template <typename Compare>
    static void merge(IntrusiveList& d, IntrusiveList& l, IntrusiveList& r, Compare&& cmp) noexcept {
        while (!l.empty() && !r.empty()) {
            if (cmp(r.front(), l.front())) {
                d.pushBack(r.popFront());
            } else {
                d.pushBack(l.popFront());
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
    static void sort(IntrusiveList& d, Compare&& cmp) noexcept {
        // length <= 1
        if (auto end = d.end(); end->next->next == end) {
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

void IntrusiveList::splitHalf(IntrusiveList& l, IntrusiveList& r) noexcept {
    IntrusiveList tmp;
    xchgWithEmptyList(tmp);
    ::split(tmp, &l, &r);
}

void IntrusiveList::sort(Compare1 cmp) noexcept {
    ::sort(*this, cmp);
}

void IntrusiveList::sort(Compare2 cmp, void* ctx) noexcept {
    ::sort(*this, [cmp, ctx](const IntrusiveNode* l, const IntrusiveNode* r) -> bool {
        return cmp(ctx, l, r);
    });
}

IntrusiveNode* IntrusiveList::popFrontOrNull() noexcept {
    if (empty()) {
        return nullptr;
    }

    return popFront();
}

IntrusiveNode* IntrusiveList::popBackOrNull() noexcept {
    if (empty()) {
        return nullptr;
    }

    return popBack();
}

void IntrusiveList::pushBack(IntrusiveList& lst) noexcept {
    if (lst.empty()) {
        // nothing to do
    } else if (empty()) {
        lst.xchgWithEmptyList(*this);
    } else {
        link(mutBack(), lst.mutFront());
        link(lst.mutBack(), mutEnd());
        lst.head.reset();
    }
}

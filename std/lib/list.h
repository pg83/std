#pragma once

#include "node.h"

namespace stl {
    class IntrusiveList {
        IntrusiveNode head;

    public:
        ~IntrusiveList() {
            clear();
        }

        IntrusiveList() {
        }

        IntrusiveList(IntrusiveList&& r) {
            r.xchgWithEmptyList(*this);
        }

        bool empty() const {
            return head.next == &head;
        }

        void pushBack(IntrusiveNode* node) {
            insertAfter(head.prev, node);
        }

        void pushBack(IntrusiveList& lst);

        void pushFront(IntrusiveNode* node) {
            insertAfter(&head, node);
        }

        IntrusiveNode* popFront() {
            return head.next->remove();
        }

        IntrusiveNode* popFrontOrNull();

        IntrusiveNode* popBack() {
            return head.prev->remove();
        }

        IntrusiveNode* popBackOrNull();

        void clear() {
            head.remove();
        }

        IntrusiveNode* mutFront() {
            return head.next;
        }

        const IntrusiveNode* front() const {
            return head.next;
        }

        IntrusiveNode* mutBack() {
            return head.prev;
        }

        const IntrusiveNode* back() const {
            return head.prev;
        }

        IntrusiveNode* mutEnd() {
            return &head;
        }

        const IntrusiveNode* end() const {
            return &head;
        }

        bool almostEmpty() const {
            return head.almostEmpty();
        }

        unsigned length() const;

        static void insertAfter(IntrusiveNode* pos, IntrusiveNode* node);

        static void insertBefore(IntrusiveNode* pos, IntrusiveNode* node) {
            insertAfter(pos->prev, node);
        }

        void xchg(IntrusiveList& r);
        void xchgWithEmptyList(IntrusiveList& r);

        using Compare1 = bool (*)(const IntrusiveNode*, const IntrusiveNode*);
        using Compare2 = bool (*)(void*, const IntrusiveNode*, const IntrusiveNode*);

        void sort(Compare1 cmp);
        void sort(Compare2 cmp, void* ctx);

        template <typename F>
        void sort(const F& f) {
            sort([](void* ctx, const IntrusiveNode* l, const IntrusiveNode* r) -> bool {
                return (*(F*)ctx)(l, r);
            }, (void*)&f);
        }

        void splitHalf(IntrusiveList& l, IntrusiveList& r);
    };
}

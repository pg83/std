#pragma once

#include "node.h"

namespace stl {
    class IntrusiveList {
        IntrusiveNode head;

    public:
        ~IntrusiveList() noexcept {
            clear();
        }

        IntrusiveList() noexcept {
        }

        IntrusiveList(IntrusiveList&& r) noexcept {
            r.xchgWithEmptyList(*this);
        }

        bool empty() const noexcept {
            return head.next == &head;
        }

        void pushBack(IntrusiveNode* node) noexcept {
            insertAfter(head.prev, node);
        }

        void pushBack(IntrusiveList& lst) noexcept;

        void pushFront(IntrusiveNode* node) noexcept {
            insertAfter(&head, node);
        }

        IntrusiveNode* popFront() noexcept {
            return head.next->remove();
        }

        IntrusiveNode* popFrontOrNull() noexcept;

        IntrusiveNode* popBack() noexcept {
            return head.prev->remove();
        }

        IntrusiveNode* popBackOrNull() noexcept;

        void clear() noexcept {
            head.remove();
        }

        IntrusiveNode* mutFront() noexcept {
            return head.next;
        }

        const IntrusiveNode* front() const noexcept {
            return head.next;
        }

        IntrusiveNode* mutBack() noexcept {
            return head.prev;
        }

        const IntrusiveNode* back() const noexcept {
            return head.prev;
        }

        IntrusiveNode* mutEnd() noexcept {
            return &head;
        }

        const IntrusiveNode* end() const noexcept {
            return &head;
        }

        unsigned length() const noexcept;

        static void insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept;

        static void insertBefore(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
            insertAfter(pos->prev, node);
        }

        void xchg(IntrusiveList& r) noexcept;
        void xchgWithEmptyList(IntrusiveList& r) noexcept;

        using Compare1 = bool (*)(const IntrusiveNode*, const IntrusiveNode*);
        using Compare2 = bool (*)(void*, const IntrusiveNode*, const IntrusiveNode*);

        void sort(Compare1 cmp) noexcept;
        void sort(Compare2 cmp, void* ctx) noexcept;

        template <typename F>
        void sort(const F& f) noexcept {
            sort([](void* ctx, const IntrusiveNode* l, const IntrusiveNode* r) -> bool {
                return (*(F*)ctx)(l, r);
            }, (void*)&f);
        }

        void appendChain(IntrusiveNode& node) noexcept;
        void splitHalf(IntrusiveList& l, IntrusiveList& r) noexcept;
    };
}

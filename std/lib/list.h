#pragma once

#include "node.h"

namespace Std {
    class IntrusiveList {
        IntrusiveNode head;

    public:
        inline ~IntrusiveList() noexcept {
            clear();
        }

        inline IntrusiveList() noexcept {
        }

        inline IntrusiveList(IntrusiveList&& r) noexcept {
            r.xchgWithEmptyList(*this);
        }

        inline bool empty() const noexcept {
            return head.next == &head;
        }

        inline void pushBack(IntrusiveNode* node) noexcept {
            insertAfter(head.prev, node);
        }

        inline void pushFront(IntrusiveNode* node) noexcept {
            insertAfter(&head, node);
        }

        inline IntrusiveNode* popFront() noexcept {
            IntrusiveNode* node = head.next;
            node->remove();
            return node;
        }

        inline IntrusiveNode* popBack() noexcept {
            IntrusiveNode* node = head.prev;
            node->remove();
            return node;
        }

        inline void clear() noexcept {
            head.remove();
        }

        inline IntrusiveNode* mutFront() noexcept {
            return head.next;
        }

        inline const IntrusiveNode* front() const noexcept {
            return head.next;
        }

        inline IntrusiveNode* mutBack() noexcept {
            return head.prev;
        }

        inline const IntrusiveNode* back() const noexcept {
            return head.prev;
        }

        inline IntrusiveNode* mutEnd() noexcept {
            return &head;
        }

        inline const IntrusiveNode* end() const noexcept {
            return &head;
        }

        unsigned length() const noexcept;

        static void insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept;

        static inline void insertBefore(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
            insertAfter(pos->prev, node);
        }

        void xchg(IntrusiveList& r) noexcept;
        void xchgWithEmptyList(IntrusiveList& r) noexcept;

        void sort(bool (*cmp)(const IntrusiveNode*, const IntrusiveNode*)) noexcept;
    };
}

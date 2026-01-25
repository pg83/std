#pragma once

namespace Std {
    struct IntrusiveNode {
        IntrusiveNode* prev;
        IntrusiveNode* next;

        IntrusiveNode()
            : prev(this)
            , next(this)
        {
        }

        void remove() noexcept {
            prev->next = next;
            next->prev = prev;
            prev = this;
            next = this;
        }
    };

    static void insertAfter(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
        node->next = pos->next;
        node->prev = pos;
        pos->next->prev = node;
        pos->next = node;
    }

    static void insertBefore(IntrusiveNode* pos, IntrusiveNode* node) noexcept {
        insertAfter(pos->prev, node);
    }

    class IntrusiveList {
        IntrusiveNode head;

    public:
        inline ~IntrusiveList() noexcept {
            clear();
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
            while (!empty()) {
                popFront();
            }
        }

        inline IntrusiveNode* front() noexcept {
            return head.next;
        }

        inline IntrusiveNode* back() noexcept {
            return head.prev;
        }

        inline IntrusiveNode* getHead() noexcept {
            return &head;
        }
    };
}

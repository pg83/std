#pragma once

namespace Std {
    struct IntrusiveNode {
        IntrusiveNode* prev;
        IntrusiveNode* next;

        inline IntrusiveNode() noexcept
            : prev(this)
            , next(this)
        {
        }

        inline auto remove() noexcept {
            prev->next = next;
            next->prev = prev;
            prev = this;
            next = this;

            return this;
        }

        void xchg(IntrusiveNode& r);
    };
}

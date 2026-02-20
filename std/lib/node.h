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
            return (unlink(), this);
        }

        void unlink() noexcept;
        void xchg(IntrusiveNode& r);
    };
}

#pragma once

namespace Std {
    struct IntrusiveNode {
        IntrusiveNode* prev = this;
        IntrusiveNode* next = this;

        inline auto remove() noexcept {
            return (unlink(), this);
        }

        void unlink() noexcept;
        void xchg(IntrusiveNode& r);
    };
}

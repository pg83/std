#pragma once

namespace Std {
    struct IntrusiveNode {
        IntrusiveNode* prev = this;
        IntrusiveNode* next = this;

        inline auto remove() noexcept {
            return (unlink(), this);
        }

        void reset() noexcept;
        void unlink() noexcept;
        void xchg(IntrusiveNode& r);

        bool singular() const noexcept;
    };
}

#pragma once

namespace stl {
    struct IntrusiveNode {
        IntrusiveNode* prev = this;
        IntrusiveNode* next = this;

        auto remove() noexcept {
            return (unlink(), this);
        }

        void reset() noexcept;
        void unlink() noexcept;
        void fixPrev() noexcept;
        void xchg(IntrusiveNode& r);

        bool singular() const noexcept;
        bool almostEmpty() const noexcept;
    };
}

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
        void xchg(IntrusiveNode& r);

        bool singular() const noexcept;
        void fixPrev() noexcept;
    };
}

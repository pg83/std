#pragma once

namespace stl {
    struct IntrusiveNode {
        IntrusiveNode* prev = this;
        IntrusiveNode* next = this;

        auto remove() {
            return (unlink(), this);
        }

        void reset();
        void unlink();
        void fixPrev();
        void xchg(IntrusiveNode& r);

        bool singular() const;
        bool almostEmpty() const;
    };
}

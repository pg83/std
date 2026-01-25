#pragma once

namespace Std {
    struct IntrusiveNode {
        IntrusiveNode* prev;
        IntrusiveNode* next;

        IntrusiveNode() noexcept
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
}

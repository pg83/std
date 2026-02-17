#pragma once

namespace Std {
    struct TreapNode {
        int priority;
        TreapNode* left;
        TreapNode* right;

        TreapNode() noexcept;

        virtual void* key() noexcept;
    };
}

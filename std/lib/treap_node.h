#pragma once

#include <std/sys/types.h>

namespace Std {
    struct TreapVisitor;

    struct TreapNode {
        u64 priority;
        TreapNode* left;
        TreapNode* right;

        TreapNode() noexcept;
        TreapNode(u64 prio) noexcept;

        virtual void* key() const noexcept;

        void visit(TreapVisitor& vis);
        size_t height() const noexcept;
    };
}

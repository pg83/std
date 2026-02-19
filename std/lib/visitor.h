#pragma once

namespace Std {
    struct VisitorFace {
        virtual void visit(void*) = 0;
    };

    template <typename V>
    struct Visitor: public VisitorFace {
        V v;

        inline Visitor(V vv)
            : v(vv)
        {
        }

        void visit(void* el) noexcept {
            v(el);
        }
    };
}

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

        void operator()(void* el) {
            visit(el);
        }

        void visit(void* el) {
            v(el);
        }
    };

    template <typename T>
    inline Visitor<T> makeVisitor(T t) {
        return Visitor<T>{t};
    }
}

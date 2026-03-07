#pragma once

namespace stl {
    struct VisitorFace {
        virtual void visit(void*) = 0;
    };

    template <typename V>
    struct Visitor: public VisitorFace {
        V v;

        Visitor(V vv)
            : v(vv)
        {
        }

        void visit(void* el) override {
            v(el);
        }
    };

    template <typename T>
    Visitor<T> makeVisitor(T t) {
        return {t};
    }
}

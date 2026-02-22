#pragma once

#include "h_table.h"

#include <std/lib/visitor.h>

namespace Std {
    class StringView;

    class SymbolTable {
        HashTable htable;

    public:
        SymbolTable();
        ~SymbolTable() noexcept;

        template <typename V>
        inline void visit(V v) {
            htable.visit(v);
        }

        inline void compactify() {
            htable.compactify();
        }

        void* set(StringView key, void* v);
        void* find(StringView key) const noexcept;
    };
}

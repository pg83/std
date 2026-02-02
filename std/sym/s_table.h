#pragma once

#include "h_table.h"

namespace Std {
    class StringView;

    class SymbolTable {
        HashTable htable;

    public:
        using Iterator = HashTable::Iterator;

        SymbolTable();
        ~SymbolTable() noexcept;

        inline void forEach(Iterator& it) {
            htable.forEach(it);
        }

        void compactify() {
            htable.compactify();
        }

        void set(const StringView& key, void* v);
        void* find(const StringView& key) const noexcept;
    };
}

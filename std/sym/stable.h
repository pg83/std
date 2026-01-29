#pragma once

#include "h_table.h"

namespace Std {
    class StringView;

    class SymbolTable {
        HashTable htable;

    public:
        SymbolTable();
        ~SymbolTable() noexcept;

        void set(const StringView& key, void* v);
        void* find(const StringView& key) const noexcept;
    };
}

#pragma once

#include "htable.h"

namespace Std {
    class StringView;

    class StringTable {
        HashTable htable;

    public:
        StringTable();
        ~StringTable() noexcept;

        void set(const StringView& key, void* v);
        void* find(const StringView& key) const noexcept;
    };
}

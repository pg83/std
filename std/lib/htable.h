#pragma once

#include <std/sys/types.h>

namespace Std {
    class HashTable {
        struct Entry {
            u64 key;
            void* value;

            inline bool filled() const noexcept {
                return value;
            }
        };

        Entry* table;
        size_t capacity;
        size_t size;

        void rehash();

    public:
        HashTable(size_t initial);

        HashTable()
            : HashTable(16)
        {
        }

        ~HashTable() noexcept;

        void* find(u64 key) const noexcept;
        void set(u64 key, void* value);

        inline size_t getSize() const noexcept {
            return size;
        }

        inline size_t getCapacity() const noexcept {
            return capacity;
        }

        void xchg(HashTable& t) noexcept;
    };
}

#pragma once

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace Std {
    class HashTable {
        Buffer buf;

        void rehash();

    public:
        struct Iterator {
            virtual void process(void** el) = 0;
        };

        HashTable(size_t initial);

        HashTable()
            : HashTable(8)
        {
        }

        ~HashTable() noexcept;

        void* find(u64 key) const noexcept;
        void set(u64 key, void* value);

        inline size_t size() const noexcept {
            return buf.used();
        }

        size_t capacity() const noexcept;

        inline void xchg(HashTable& t) noexcept {
            buf.xchg(t.buf);
        }

        void forEach(Iterator& it);
    };
}

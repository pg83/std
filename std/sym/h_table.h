#pragma once

#include <std/sys/types.h>
#include <std/lib/buffer.h>

namespace Std {
    class HashTable {
        Buffer buf;

        void rehash();
        void rehashImpl(size_t initial);
        void setNoRehash(u64 key, void* value);
        void* findEntryPtr(u64 key) const noexcept;

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

        inline void xchg(HashTable& t) noexcept {
            buf.xchg(t.buf);
        }

        inline size_t size() const noexcept {
            return buf.used();
        }

        void* find(u64 key) const noexcept;
        size_t capacity() const noexcept;
        void set(u64 key, void* value);
        void erase(u64 key) noexcept;
        void forEach(Iterator& it);
        void compactify();
    };
}

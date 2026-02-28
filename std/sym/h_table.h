#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace Std {
    class HashTable {
        struct Impl;
        Impl* impl;

        void rehash(size_t len);
        void visitImpl(VisitorFace&& v);

    public:
        HashTable(size_t initial);

        inline HashTable()
            : HashTable(8)
        {
        }

        ~HashTable() noexcept;

        void* find(u64 key) const noexcept;
        void xchg(HashTable& t) noexcept;
        size_t capacity() const noexcept;
        void* set(u64 key, void* value);
        void* erase(u64 key) noexcept;
        size_t size() const noexcept;

        template <typename V>
        inline void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((void**)ptr);
            }));
        }
    };
}

#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace Std {
    class HashTable {
    public:
        struct Node {
            u64 key;
            Node* next = nullptr;
        };

    private:
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

        Node* find(u64 key) const noexcept;
        void xchg(HashTable& t) noexcept;
        size_t capacity() const noexcept;
        Node* insert(Node*);
        Node* erase(u64 key) noexcept;
        size_t size() const noexcept;

        template <typename V>
        inline void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((Node*)ptr);
            }));
        }

        static inline u64 hash(u64 k) noexcept {
            return k;
        }
    };
}

#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace Std {
    class HashTable2 {
    public:
        struct Node {
            u64 key;
            Node* next;
        };

    private:
        struct Impl;
        Impl* impl;

        void rehash(size_t len);
        void visitImpl(VisitorFace&& v);

    public:
        HashTable2(size_t initial);

        inline HashTable2()
            : HashTable2(8)
        {
        }

        ~HashTable2() noexcept;

        Node* find(u64 key) const noexcept;
        void xchg(HashTable2& t) noexcept;
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
    };
}

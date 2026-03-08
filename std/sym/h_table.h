#pragma once

#include <std/sys/types.h>
#include <std/lib/buffer.h>
#include <std/lib/visitor.h>

namespace stl {
    class HashTable {
    public:
        struct Node {
            u64 key;
            Node* next = nullptr;
        };

    private:
        Buffer buf;

        void rehash(size_t len);
        void addNoRehash(Node* node);
        void visitImpl(VisitorFace&& v);
        Node** bucketFor(u64 key) const;

    public:
        HashTable(size_t initial);

        HashTable()
            : HashTable(8)
        {
        }

        ~HashTable();

        Node* find(u64 key) const;
        void xchg(HashTable& t);
        size_t capacity() const;
        Node* erase(u64 key);
        size_t size() const;
        Node* insert(Node* node);

        template <typename V>
        void visit(V v) {
            visitImpl(makeVisitor([v](void* ptr) {
                v((Node*)ptr);
            }));
        }
    };
}

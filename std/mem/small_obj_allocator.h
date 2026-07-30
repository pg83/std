#pragma once

#include "new.h"
#include "embed.h"

#include <std/sys/types.h>
#include <std/alg/destruct.h>

namespace stl {
    class ObjPool;

    inline constexpr size_t smallObjMaxSize = 2048;

    class SmallObjAllocator {
    public:
        virtual ~SmallObjAllocator() noexcept;

        virtual void* allocate(size_t size) = 0;
        virtual void deallocate(void* pointer, size_t size) noexcept = 0;

        template <typename T, typename... Args>
        T* make(Args&&... args) {
            struct Storage: public Embed<T>, public Newable {
                using Embed<T>::Embed;
            };

            static_assert(sizeof(Storage) == sizeof(T));
            static_assert(sizeof(T) <= smallObjMaxSize);
            static_assert(alignof(T) <= alignof(max_align_t));

            return &(new (allocate(sizeof(T))) Storage(forward<Args>(args)...))->t;
        }

        template <typename T>
        void release(T* object) noexcept {
            deallocate(destruct(object), sizeof(T));
        }

        static SmallObjAllocator* create(ObjPool* pool);
    };
}

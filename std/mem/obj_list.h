#pragma once

#include "new.h"
#include "embed.h"
#include "free_list.h"

#include <std/alg/destruct.h>
#include <std/typ/support.h>

namespace Std {
    template <typename T>
    class ObjList {
        struct TT: public Embed<T>, public Newable {
            using Embed<T>::Embed;
        };

        static_assert(sizeof(TT) == sizeof(T));

        FreeList::Ref fl = FreeList::fromMemory(sizeof(TT));

    public:
        template <typename... A>
        inline T* make(A&&... a) {
            return &(new (fl->allocate()) TT(forward<A>(a)...))->t;
        }

        inline void release(T* t) {
            fl->release(destruct((TT*)(void*)t));
        }
    };
}

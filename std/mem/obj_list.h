#pragma once

#include "new.h"
#include "embed.h"
#include "free_list.h"

#include <std/typ/support.h>

namespace Std {
    template <typename T>
    class ObjList {
        struct TT: public T, public Newable {
            using T::T;
        };

        static_assert(sizeof(TT) == sizeof(T));

        FreeList::Ref fl = FreeList::fromMemory(sizeof(TT));

    public:
        template <typename... A>
        inline T* make(A&&... a) {
            return new (fl->allocate()) TT(forward<A>(a)...);
        }

        inline void release(T* t) {
            auto tt = (TT*)t;
            tt->~TT();
            fl->release(tt);
        }
    };
}

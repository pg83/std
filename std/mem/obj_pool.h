#pragma once

#include "new.h"
#include "embed.h"
#include "disposable.h"

#include <std/sys/types.h>

#include <std/typ/intrin.h>

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace Std {
    class StringView;

    class ObjPool: public ARC {
        virtual void submit(Disposable* d) noexcept = 0;

        template <typename T, typename... A>
        inline T* makeImpl(A&&... a) {
            return new (allocate(sizeof(T))) T(forward<A>(a)...);
        }

    public:
        using Ref = IntrusivePtr<ObjPool>;

        virtual ~ObjPool() noexcept;

        virtual void* allocate(size_t len) = 0;

        StringView intern(const StringView& s);

        // king of ownership
        template <typename T, typename... A>
        inline T* make(A&&... a) {
            struct Wrapper1: public Embed<T>, public Newable {
                using Embed<T>::Embed;
            };

            struct Wrapper2: public Disposable, public Wrapper1 {
                using Wrapper1::Wrapper1;
            };

            if constexpr (stdHasTrivialDestructor(T)) {
                static_assert(sizeof(Wrapper1) == sizeof(T));

                return &makeImpl<Wrapper1>(forward<A>(a)...)->t;
            } else {
                auto res = makeImpl<Wrapper2>(forward<A>(a)...);

                submit(res);

                return &res->t;
            }
        }

        static Ref fromMemory();
    };
}

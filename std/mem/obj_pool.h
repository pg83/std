#pragma once

#include "embed.h"
#include "disposable.h"

#include <std/sys/types.h>

#include <std/typ/intrin.h>

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace Std {
    class StringView;

    class ObjPool: public ARC {
        template <typename T>
        struct Wrapper1: public Embed<T> {
            using Embed<T>::Embed;

            static void* operator new(size_t, void* ptr) noexcept {
                return ptr;
            }
        };

        template <typename T>
        struct Wrapper2: public Disposable, public Wrapper1<T> {
            using Wrapper1<T>::Wrapper1;
        };

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
            if constexpr (stdHasTrivialDestructor(T)) {
                return &makeImpl<Wrapper1<T>>(forward<A>(a)...)->t;
            } else {
                auto res = makeImpl<Wrapper2<T>>(forward<A>(a)...);

                submit(res);

                return &res->t;
            }
        }

        static Ref fromMemory();
    };
}

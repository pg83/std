#pragma once

#include "new.h"
#include "embed.h"
#include "disposable.h"

#include <std/ptr/arc.h>
#include <std/sys/types.h>
#include <std/typ/intrin.h>
#include <std/ptr/intrusive.h>

namespace stl {
    class MemoryPool;
    class StringView;

    class ObjPool: public ARC {
        virtual void submit(Disposable* d) noexcept = 0;

        template <size_t Size, size_t Align>
        void* allocFor() {
            if constexpr (Align > alignof(max_align_t)) {
                return allocateOverAligned(Size, Align);
            } else {
                return allocate(Size);
            }
        }

        template <typename T, typename... A>
        T* makeImpl(A&&... a) {
            return new (allocFor<sizeof(T), alignof(T)>()) T(forward<A>(a)...);
        }

    public:
        using Ref = IntrusivePtr<ObjPool>;

        virtual ~ObjPool() noexcept;

        virtual void* allocate(size_t len) = 0;
        virtual MemoryPool* memoryPool() noexcept = 0;

        StringView intern(StringView s);
        void* allocateOverAligned(size_t len, size_t align);

        // king of ownership
        template <typename T, typename... A>
        T* make(A&&... a) {
            struct Wrapper1: public Embed<T>, public Newable {
                using Embed<T>::Embed;
            };

            if constexpr (stdHasTrivialDestructor(T)) {
                static_assert(sizeof(Wrapper1) == sizeof(T));

                return &makeImpl<Wrapper1>(forward<A>(a)...)->t;
            } else {
                struct Wrapper2: public Disposable, public Wrapper1 {
                    using Wrapper1::Wrapper1;
                };

                auto res = makeImpl<Wrapper2>(forward<A>(a)...);

                submit(res);

                return &res->t;
            }
        }

        static Ref fromMemory() {
            return fromMemoryRaw();
        }

        static ObjPool* create(ObjPool* pool);
        static ObjPool* fromMemoryRaw();
    };
}

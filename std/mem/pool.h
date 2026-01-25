#pragma once

#include <std/sys/types.h>

#include <std/typ/meta.h>
#include <std/typ/traits.h>
#include <std/typ/support.h>

#include <std/lib/node.h>

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace Std {
    class StringView;

    struct Pool: public ARC {
        struct Dispose: public IntrusiveNode {
            virtual ~Dispose() noexcept;
        };

        template <typename T>
        struct Wrapper: public Dispose {
            T t;

            static void* operator new(size_t, void* ptr) {
                return ptr;
            }

            template <typename... A>
            inline Wrapper(A&&... a)
                : t(forward<A>(a)...)
            {
            }
        };

        using Ref = IntrusivePtr<Pool>;

        virtual ~Pool();

        virtual void* allocate(size_t len) = 0;
        virtual void submit(Dispose* d) noexcept = 0;

        StringView intern(const StringView& s);

        // king of ownership
        template <typename T, typename... A>
        inline Meta::EnableIf<Traits::HasDestructor<T>::R, T*> make(A&&... a) {
            using Typ = Wrapper<T>;

            auto mem = this->allocate(sizeof(Typ));
            auto res = new (mem) Typ(forward<A>(a)...);

            submit(res);

            return &res->t;
        }

        template <typename T, typename... A>
        inline Meta::EnableIf<!Traits::HasDestructor<T>::R, T*> make(A&&... a) {
            using Typ = Wrapper<T>;

            return &(new (allocate(sizeof(Typ))) Typ(forward<A>(a)...))->t;
        }

        static Ref fromMemory();
    };
}

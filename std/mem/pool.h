#pragma once

#include <std/sys/types.h>

#include <std/typ/support.h>

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace Std {
    class StringView;

    struct Pool: public ARC {
        struct Dispose {
            virtual ~Dispose();
        };

        template <typename T>
        struct Wrapper: public Dispose {
            T t;

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

        template <typename T, typename... A>
        inline auto make(A&&... a) {
            auto res = new Wrapper<T>(forward<A>(a)...);

            submit(res);

            return &res->t;
        }

        static Ref fromMemory();
    };
}

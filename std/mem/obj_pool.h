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

    class ObjPool: public ARC {
    public:
        struct Dispose: public IntrusiveNode {
            virtual ~Dispose() noexcept;
        };

    private:
        template <typename T>
        struct Wrapper1 {
            T t;

            static void* operator new(size_t, void* ptr) {
                return ptr;
            }

            template <typename... A>
            inline Wrapper1(A&&... a)
                : t(forward<A>(a)...)
            {
            }
        };

        template <typename T>
        struct Wrapper2: public Dispose, public Wrapper1<T> {
            using Wrapper1<T>::Wrapper1;
        };

        virtual void submit(Dispose* d) noexcept = 0;

    public:
        using Ref = IntrusivePtr<ObjPool>;

        virtual ~ObjPool() noexcept;

        virtual void* allocate(size_t len) = 0;

        StringView intern(const StringView& s);

        // king of ownership
        template <typename T, typename... A>
        inline Meta::EnableIf<Traits::HasDestructor<T>::R, T*> make(A&&... a) {
            using Typ = Wrapper2<T>;

            auto mem = this->allocate(sizeof(Typ));
            auto res = new (mem) Typ(forward<A>(a)...);

            submit(res);

            return &res->t;
        }

        template <typename T, typename... A>
        inline Meta::EnableIf<!Traits::HasDestructor<T>::R, T*> make(A&&... a) {
            using Typ = Wrapper1<T>;

            return &(new (allocate(sizeof(Typ))) Typ(forward<A>(a)...))->t;
        }

        static Ref fromMemory();
    };
}

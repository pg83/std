#pragma once

#include "future.h"
#include "coro.h"

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace stl {
    template <typename T>
    class SharedFutureBase: public ARC {
        struct TT {
            T t;

            void* operator new(size_t, void* p) noexcept {
                return p;
            }
        };

        Future f;
        alignas(T) char buf[sizeof(T)];

    public:
        SharedFutureBase() noexcept = default;

        explicit SharedFutureBase(CoroExecutor* exec) noexcept
            : f(exec)
        {
        }

        ~SharedFutureBase() noexcept {
            ((TT*)f.posted())->~TT();
        }

        void post(T&& t) noexcept {
            f.post(new (buf) TT{.t = static_cast<T&&>(t)});
        }

        T& wait() noexcept {
            return ((TT*)f.wait())->t;
        }
    };

    template <typename T>
    using SharedFuture = IntrusivePtr<SharedFutureBase<T>>;

    template <typename F>
    auto awaitable(CoroExecutor* exec, F fn) {
        auto sf = makeIntrusivePtr(new SharedFutureBase<decltype(fn())>(exec));

        exec->spawn([sf, fn]() mutable {
            sf->post(fn());
        });

        return sf;
    }
}

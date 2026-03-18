#pragma once

#include "coro.h"
#include "future.h"

#include <std/ptr/arc.h>
#include <std/ptr/intrusive.h>

namespace stl {
    template <typename T>
    class SharedFutureBase: public ARC {
        Future f;

    public:
        SharedFutureBase() noexcept = default;

        explicit SharedFutureBase(CoroExecutor* exec) noexcept
            : f(exec)
        {
        }

        ~SharedFutureBase() noexcept {
            delete posted();
        }

        void post(T&& t) noexcept {
            f.post(new T(static_cast<T&&>(t)));
        }

        T& wait() noexcept {
            return *(T*)f.wait();
        }

        auto posted() noexcept {
            return (T*)f.posted();
        }

        auto release() noexcept {
            return (T*)f.release();
        }

        auto consume() noexcept {
            return (wait(), release());
        }
    };

    template <typename T>
    using SharedFuture = IntrusivePtr<SharedFutureBase<T>>;

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        auto sf = makeIntrusivePtr(
            exec->me()
                ? new SharedFutureBase<decltype(fn())>(exec)
                : new SharedFutureBase<decltype(fn())>() //
        );

        exec->spawn([sf, fn]() mutable {
            sf->post(fn());
        });

        return sf;
    }
}

#pragma once

#include <std/lib/producer.h>
#include <std/ptr/intrusive.h>
#include <std/sys/types.h>

namespace stl {
    struct CoroExecutor;

    struct FutureIface {
        virtual ~FutureIface() noexcept;

        virtual i32 ref() noexcept = 0;
        virtual i32 unref() noexcept = 0;
        virtual i32 refCount() const noexcept = 0;

        virtual void* wait() noexcept = 0;
        virtual void* posted() noexcept = 0;
        virtual void* release() noexcept = 0;
    };

    FutureIface* asyncImpl(CoroExecutor* exec, ProducerIface* prod);

    template <typename T>
    class SharedFuture {
        IntrusivePtr<FutureIface> impl_;

    public:
        SharedFuture(FutureIface* p) noexcept
            : impl_(p)
        {
        }

        T& wait() noexcept {
            return *(T*)impl_->wait();
        }

        T* posted() noexcept {
            return (T*)impl_->posted();
        }

        T* release() noexcept {
            return (T*)impl_->release();
        }

        T* consume() noexcept {
            return (wait(), release());
        }
    };

    template <typename F>
    auto async(CoroExecutor* exec, F fn) {
        using T = decltype(fn());
        return SharedFuture<T>(asyncImpl(exec, makeProducer(fn)));
    }
}

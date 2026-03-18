#include "async.h"
#include "coro.h"
#include "pool.h"
#include "semaphore.h"

#include <std/ptr/arc.h>
#include <std/alg/exchange.h>

using namespace stl;

namespace {
    struct FutureImpl: public FutureIface {
        ARC arc;
        Semaphore sem;
        void* value;
        ProducerIface* prod;

        FutureImpl(ProducerIface* p) noexcept
            : sem(0)
            , value(nullptr)
            , prod(p)
        {
        }

        FutureImpl(CoroExecutor* exec, ProducerIface* p) noexcept
            : sem(0, exec)
            , value(nullptr)
            , prod(p)
        {
        }

        ~FutureImpl() noexcept override {
            prod->del(value);
            delete prod;
        }

        i32 ref() noexcept override {
            return arc.ref();
        }

        i32 unref() noexcept override {
            return arc.unref();
        }

        i32 refCount() const noexcept override {
            return arc.refCount();
        }

        void* wait() noexcept override {
            sem.wait();
            return value;
        }

        void* posted() noexcept override {
            return value;
        }

        void* release() noexcept override {
            return exchange(value, nullptr);
        }

        void execute() {
            value = prod->run();
            sem.post();
        }
    };
}

FutureIfaceRef stl::asyncImpl(ProducerIface* prod, ThreadPool* pool) {
    auto fi = makeIntrusivePtr(new FutureImpl(prod));

    pool->submit([fi]() mutable {
        fi->execute();
    });

    return fi.mutPtr();
}

FutureIfaceRef stl::asyncImpl(ProducerIface* prod, CoroExecutor* exec) {
    auto fi = makeIntrusivePtr(exec->me() ? new FutureImpl(exec, prod) : new FutureImpl(prod));

    exec->spawn([fi]() mutable {
        fi->execute();
    });

    return fi.mutPtr();
}

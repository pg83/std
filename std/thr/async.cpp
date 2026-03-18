#include "async.h"
#include "coro.h"
#include "future.h"

#include <std/ptr/arc.h>

using namespace stl;

namespace {
    struct FutureImpl: public FutureIface {
        ARC arc;
        Future f;
        ProducerIface* prod;

        FutureImpl(ProducerIface* p) noexcept
            : prod(p)
        {
        }

        FutureImpl(CoroExecutor* exec, ProducerIface* p) noexcept
            : f(exec)
            , prod(p)
        {
        }

        ~FutureImpl() noexcept override {
            prod->del(f.posted());
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
            return f.wait();
        }

        void* posted() noexcept override {
            return f.posted();
        }

        void* release() noexcept override {
            return f.release();
        }
    };
}

FutureIface::~FutureIface() noexcept = default;

FutureIface* stl::asyncImpl(CoroExecutor* exec, ProducerIface* prod) {
    auto* fi = exec->me()
        ? new FutureImpl(exec, prod)
        : new FutureImpl(prod);

    exec->spawn([fi]() {
        fi->f.post(fi->prod->run());
    });

    return fi;
}

#include "reactor_fs.h"
#include "pool.h"

#include <std/mem/obj_pool.h>

#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>

using namespace stl;

namespace {
    struct FSReactorState: public FSReactorIface {
        ThreadPool* pool_;

        FSReactorState(ThreadPool* pool, ObjPool* opool);

        void run() noexcept override;
        void stop() noexcept override;
        void submit(FSRequest* req) override;
    };
}

FSReactorState::FSReactorState(ThreadPool* pool, ObjPool* opool)
    : pool_(pool)
{
    if (!pool_) {
        pool_ = ThreadPool::simple(opool, 4);
    }
}

void FSReactorState::run() noexcept {
}

void FSReactorState::stop() noexcept {
}

void FSReactorState::submit(FSRequest* req) {
    req->parkWith(makeRunable([this, req] {
        pool_->submit([req] {
            ssize_t n;

            if (req->op == FSRequestOp::Read) {
                n = ::preadv(req->fd, req->iov, req->iovcnt, req->offset);
            } else {
                n = ::pwritev(req->fd, req->iov, req->iovcnt, req->offset);
            }

            req->complete(n < 0 ? (i64)-errno : (i64)n);
        });
    }));
}

FSReactorIface* FSReactorIface::create(ThreadPool* pool, ObjPool* opool) {
    return opool->make<FSReactorState>(pool, opool);
}

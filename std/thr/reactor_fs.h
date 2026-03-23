#pragma once

#include "runable.h"

#include <std/lib/node.h>
#include <std/sys/types.h>

struct iovec;

namespace stl {
    class ObjPool;

    struct ThreadPool;
    struct CoroExecutor;

    namespace FSRequestOp {
        constexpr u32 Read = 0;
        constexpr u32 Write = 1;
    }

    struct FSRequest: public IntrusiveNode {
        struct iovec* iov;
        size_t iovcnt;
        off_t offset;
        i32 fd;
        u32 op;

        virtual void complete(ssize_t result) noexcept = 0;
        virtual void parkWith(Runable&& afterSuspend) noexcept = 0;
    };

    struct FSReactorIface {
        virtual void submit(FSRequest* req) = 0;

        static FSReactorIface* create(ThreadPool* pool, ObjPool* opool);
    };
}

#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct PollFD;
    struct ThreadPool;
    struct VisitorFace;
    struct CoroExecutor;

    struct PollGroup {
        virtual int fd() const noexcept = 0;

        static PollGroup* create(ObjPool* pool, const PollFD* fds, size_t count);
    };

    struct ReactorIface {
        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) = 0;

        static ReactorIface* create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool);
    };
}

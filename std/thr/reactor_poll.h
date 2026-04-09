#pragma once

#include <std/sys/types.h>

namespace stl {
    class ObjPool;

    struct PollFD;
    struct PollGroup;
    struct VisitorFace;
    struct CoroExecutor;

    struct ReactorIface {
        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
        virtual void poll(PollGroup* g, VisitorFace& visitor, u64 deadlineUs) = 0;

        static int pollGroupFd(const PollGroup* g) noexcept;
        static ReactorIface* create(CoroExecutor* exec, ObjPool* opool);
        static PollGroup* createPollGroup(ObjPool* pool, const PollFD* fds, size_t count);
    };
}

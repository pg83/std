#pragma once

#include <std/lib/node.h>
#include <std/sys/types.h>
#include <std/map/treap_node.h>

namespace stl {
    class ObjPool;

    struct Runable;
    struct ThreadPool;
    struct CoroExecutor;
    struct ReactorIface;

    struct PollRequest: public TreapNode, public IntrusiveNode {
        ReactorIface* reactor;
        int fd;
        u32 flags;
        u32 result;
        u64 deadline;

        virtual void reSchedule() noexcept = 0;
        virtual void parkWith(Runable* afterSuspend) noexcept = 0;
    };

    struct ReactorIface {
        virtual ~ReactorIface() noexcept;

        virtual void run() noexcept = 0;
        virtual void join() noexcept = 0;
        virtual void processRequest(PollRequest* req) = 0;

        static ReactorIface* create(CoroExecutor* exec, ThreadPool* pool, ObjPool* opool);
    };
}

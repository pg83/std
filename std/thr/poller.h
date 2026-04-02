#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace stl {
    class ObjPool;

    namespace PollFlag {
        constexpr u32 In = 1;
        constexpr u32 Out = 2;
        constexpr u32 Err = 4;
        constexpr u32 Hup = 8;
    }

    struct PollFD {
        int fd;
        u32 flags;
    };

    struct PollerIface {
        // add or re-arm fd with ONESHOT semantics
        virtual void arm(PollFD pfd) = 0;
        // remove fd from poller
        virtual void disarm(int fd) = 0;
        // wait for events, always finite timeout
        virtual void waitImpl(VisitorFace& v, u32 timeoutUs) = 0;

        void waitBase(VisitorFace&& v, u64 deadlineUs);

        template <typename V>
        void wait(V v, u64 deadlineUs) {
            // clang-format off
            waitBase(makeVisitor([v](void* ptr) {
                v((PollFD*)ptr);
            }), deadlineUs);
            // clang-format on
        }

        static PollerIface* create(ObjPool* pool);
    };
}

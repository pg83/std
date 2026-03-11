#pragma once

#include <std/sys/types.h>
#include <std/lib/visitor.h>

namespace stl {
    namespace PollFlag {
        constexpr u32 In = 1;
        constexpr u32 Out = 2;
        constexpr u32 Err = 4;
        constexpr u32 Hup = 8;
    }

    struct PollEvent {
        void* data;
        u32 flags;
    };

    struct PollerIface {
        virtual ~PollerIface() noexcept;

        // add or re-arm fd with ONESHOT semantics
        virtual void arm(int fd, u32 flags, void* data) = 0;
        // remove fd from poller
        virtual void disarm(int fd) = 0;
        // wait for events, always finite timeout
        virtual void waitImpl(VisitorFace&& v, u32 timeoutUs) = 0;

        template <typename V>
        void wait(V v, u32 timeoutUs) {
            waitImpl(makeVisitor([v](void* ptr) {
                v((PollEvent*)ptr);
            }), timeoutUs);
        }

        static PollerIface* create();
    };
}

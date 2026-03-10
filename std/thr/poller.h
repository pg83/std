#pragma once
#include <std/sys/types.h>

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
        // wait for events, returns count. always finite timeout
        virtual u32 wait(PollEvent* out, u32 maxEvents, u32 timeoutMs) = 0;

        static PollerIface* create();
    };
}

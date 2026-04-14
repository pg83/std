#pragma once

#include <std/sys/types.h>

namespace stl {
    namespace PollFlag {
        constexpr u32 In = 1;
        constexpr u32 Out = 4;
        constexpr u32 Err = 8;
        constexpr u32 Hup = 16;
    }

    struct PollFD {
        int fd;
        u32 flags;

        short toPollEvents() const noexcept;

        static u32 fromPollEvents(short events) noexcept;
    };
}

#pragma once

#include <std/sys/types.h>

namespace stl {
    struct PollFD;

    struct Pollable {
        virtual u32 poll(PollFD pfd, u64 deadlineUs) = 0;
    };
}

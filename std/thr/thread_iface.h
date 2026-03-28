#pragma once

#include <std/sys/types.h>

namespace stl {
    struct Runable;

    struct ThreadIface {
        virtual ~ThreadIface() noexcept;

        virtual void start(Runable& runable) = 0;
        virtual void join() noexcept = 0;
        virtual void detach() noexcept = 0;
        virtual u64 threadId() const noexcept = 0;
    };
}

#pragma once

#include <std/sys/types.h>

namespace stl {
    struct Runable;

    struct Context {
        virtual void switchTo(Context& target) noexcept = 0;

        static size_t implSize() noexcept;
        static Context* create(void* buf) noexcept;
        static Context* create(void* buf, void* stackPtr, size_t stackSize, Runable& entry) noexcept;
    };
}

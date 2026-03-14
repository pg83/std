#pragma once
#include <std/sys/types.h>

namespace stl {
    struct Context {
        static constexpr size_t kBufSize = 1024;

        virtual void switchTo(Context& target) noexcept = 0;

        static Context* create(void* buf) noexcept;
        static Context* create(void* buf, void* stackPtr, size_t stackSize,
                               void (*fn)(u32, u32), uintptr_t p) noexcept;
    };
}

#pragma once

#include <std/sys/types.h>

namespace stl {
    struct ChannelIface {
        virtual ~ChannelIface() noexcept;

        virtual void close() noexcept = 0;
        virtual void enqueue(void* v) noexcept = 0;
        virtual bool dequeue(void** out) noexcept = 0;
        virtual bool tryEnqueue(void* v) noexcept = 0;
        virtual bool tryDequeue(void** out) noexcept = 0;
    };
}

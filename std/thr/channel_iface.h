#pragma once

#include <std/sys/types.h>

namespace stl {
    struct ChannelIface {
        virtual ~ChannelIface() noexcept;

        virtual void enqueue(void* v) = 0;
        virtual bool dequeue(void** out) = 0;
        virtual bool tryEnqueue(void* v) = 0;
        virtual bool tryDequeue(void** out) = 0;
        virtual void close() = 0;
    };
}

#pragma once

#include <std/sys/types.h>

namespace stl {
    struct ChannelIface;
    struct CoroExecutor;

    class Channel {
        ChannelIface* impl_;

    public:
        explicit Channel(CoroExecutor* exec);
        Channel(CoroExecutor* exec, size_t cap);
        Channel(ChannelIface* iface) noexcept;

        ~Channel() noexcept;

        void enqueue(void* v) noexcept;
        bool dequeue(void** out) noexcept;
        bool tryEnqueue(void* v) noexcept;
        bool tryDequeue(void** out) noexcept;

        void close() noexcept;
    };
}

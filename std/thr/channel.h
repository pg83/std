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
        Channel(ChannelIface* iface);

        ~Channel() noexcept;

        void enqueue(void* v);
        bool dequeue(void** out);
        bool tryEnqueue(void* v);
        bool tryDequeue(void** out);
        void close();
    };
}

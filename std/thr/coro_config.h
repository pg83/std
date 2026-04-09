#pragma once

#include <std/sys/types.h>

namespace stl {
    struct CoroConfig {
        size_t threads;
        size_t reactors;
        size_t offloadThreads;

        CoroConfig(size_t threads) noexcept;

        CoroConfig& setThreads(size_t v) noexcept;
        CoroConfig& setReactors(size_t v) noexcept;
        CoroConfig& setOffloadThreads(size_t v) noexcept;
    };
}

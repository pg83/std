#include "coro_config.h"

using namespace stl;

CoroConfig::CoroConfig(size_t threads) noexcept
    : threads(threads)
    , reactors(threads)
{
}

CoroConfig& CoroConfig::setThreads(size_t v) noexcept {
    threads = v;

    return *this;
}

CoroConfig& CoroConfig::setReactors(size_t v) noexcept {
    reactors = v;

    return *this;
}

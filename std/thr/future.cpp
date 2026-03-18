#include "future.h"

#include <std/alg/exchange.h>

using namespace stl;

Future::Future() noexcept
    : sem_(0)
    , value_(nullptr)
{
}

Future::Future(CoroExecutor* exec) noexcept
    : sem_(0, exec)
    , value_(nullptr)
{
}

Future::~Future() noexcept {
}

void Future::post(void* value) noexcept {
    value_ = value;
    sem_.post();
}

void* Future::wait() noexcept {
    sem_.wait();
    return value_;
}

void* Future::release() noexcept {
    return exchange(value_, nullptr);
}

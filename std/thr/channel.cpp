#include "channel.h"
#include "coro.h"
#include "channel_iface.h"

using namespace stl;

Channel::Channel(CoroExecutor* exec)
    : Channel(exec, 0)
{
}

Channel::Channel(CoroExecutor* exec, size_t cap)
    : Channel(exec->createChannel(cap))
{
}

Channel::Channel(ChannelIface* iface) noexcept
    : impl_(iface)
{
}

Channel::~Channel() noexcept {
    delete impl_;
}

void Channel::enqueue(void* v) noexcept {
    impl_->enqueue(v);
}

bool Channel::dequeue(void** out) noexcept {
    return impl_->dequeue(out);
}

bool Channel::tryEnqueue(void* v) noexcept {
    return impl_->tryEnqueue(v);
}

bool Channel::tryDequeue(void** out) noexcept {
    return impl_->tryDequeue(out);
}

void Channel::close() noexcept {
    impl_->close();
}

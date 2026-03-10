#include "channel.h"
#include "channel_iface.h"
#include "coro.h"

using namespace stl;

Channel::Channel(CoroExecutor* exec)
    : Channel(exec, 0)
{
}

Channel::Channel(CoroExecutor* exec, size_t cap)
    : impl_(exec->createChannel(cap))
{
}

Channel::Channel(ChannelIface* iface)
    : impl_(iface)
{
}

Channel::~Channel() noexcept {
    delete impl_;
}

void Channel::enqueue(void* v) {
    impl_->enqueue(v);
}

bool Channel::dequeue(void** out) {
    return impl_->dequeue(out);
}

bool Channel::tryEnqueue(void* v) {
    return impl_->tryEnqueue(v);
}

bool Channel::tryDequeue(void** out) {
    return impl_->tryDequeue(out);
}

void Channel::close() {
    impl_->close();
}

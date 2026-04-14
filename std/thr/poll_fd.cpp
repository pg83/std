#include "poll_fd.h"

#include <poll.h>

using namespace stl;

short PollFD::toPollEvents() const noexcept {
    short r = 0;

    if (flags & PollFlag::In) {
        r |= POLLIN;
    }

    if (flags & PollFlag::Out) {
        r |= POLLOUT;
    }

    if (flags & PollFlag::Err) {
        r |= POLLERR;
    }

    if (flags & PollFlag::Hup) {
        r |= POLLHUP;
    }

    return r;
}

u32 PollFD::fromPollEvents(short events) noexcept {
    u32 r = 0;

    if (events & POLLIN) {
        r |= PollFlag::In;
    }

    if (events & POLLOUT) {
        r |= PollFlag::Out;
    }

    if (events & POLLERR) {
        r |= PollFlag::Err;
    }

    if (events & POLLHUP) {
        r |= PollFlag::Hup;
    }

    return r;
}

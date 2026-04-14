#include "poll_fd.h"

#include <poll.h>

using namespace stl;

#if defined(__linux__)
static_assert(PollFlag::In == POLLIN);
static_assert(PollFlag::Out == POLLOUT);
static_assert(PollFlag::Err == POLLERR);
static_assert(PollFlag::Hup == POLLHUP);

short PollFD::toPollEvents() const noexcept {
    return (short)flags;
}

u32 PollFD::fromPollEvents(short events) noexcept {
    return (u32)events & (PollFlag::In | PollFlag::Out | PollFlag::Err | PollFlag::Hup);
}
#else
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
#endif

#include "event_fd.h"

#include <std/sys/fd.h>
#include <std/str/view.h>
#include <std/sys/throw.h>
#include <std/str/builder.h>

#include <unistd.h>

#if defined(__linux__)
    #include <sys/eventfd.h>
#endif

using namespace stl;

struct EventFD::Impl {
#if defined(__linux__)
    ScopedFD efd;

    Impl() {
        int r = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

        if (r < 0) {
            Errno().raise(StringBuilder() << StringView(u8"eventfd() failed"));
        }

        ScopedFD(r).xchg(efd);
    }

    int fd() const noexcept {
        return efd.get();
    }

    void signal() {
        u64 val = 1;
        ::write(efd.get(), &val, sizeof(val));
    }

    void drain() {
        u64 val;
        ::read(efd.get(), &val, sizeof(val));
    }
#else
    ScopedFD rfd;
    ScopedFD wfd;

    Impl() {
        createPipeFD(rfd, wfd);
        rfd.setNonBlocking();
        wfd.setNonBlocking();
    }

    int fd() const noexcept {
        return rfd.get();
    }

    void signal() {
        char val = 1;
        ::write(wfd.get(), &val, sizeof(val));
    }

    void drain() {
        char buf[256];

        while (::read(rfd.get(), buf, sizeof(buf)) > 0) {
        }
    }
#endif
};

EventFD::EventFD()
    : impl(new Impl())
{
}

EventFD::~EventFD() noexcept {
    delete impl;
}

int EventFD::fd() const noexcept {
    return impl->fd();
}

void EventFD::signal() {
    impl->signal();
}

void EventFD::drain() {
    impl->drain();
}

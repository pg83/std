#include "event_fd.h"

#include <std/tst/ut.h>
#include <std/dbg/insist.h>

#include <poll.h>

using namespace stl;

static bool isReadable(int fd) {
    struct pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;
    return poll(&pfd, 1, 0) > 0;
}

STD_TEST_SUITE(EventFD) {
    STD_TEST(FdIsValid) {
        EventFD ev;
        STD_INSIST(ev.fd() >= 0);
    }

    STD_TEST(SignalMakesReadable) {
        EventFD ev;
        STD_INSIST(!isReadable(ev.fd()));
        ev.signal();
        STD_INSIST(isReadable(ev.fd()));
    }

    STD_TEST(DrainClearsSignal) {
        EventFD ev;
        ev.signal();
        ev.drain();
        STD_INSIST(!isReadable(ev.fd()));
    }

    STD_TEST(MultipleSignalsDrained) {
        EventFD ev;
        ev.signal();
        ev.signal();
        ev.signal();
        ev.drain();
        STD_INSIST(!isReadable(ev.fd()));
    }
}

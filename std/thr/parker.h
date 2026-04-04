#pragma once

#include <std/sys/event_fd.h>

namespace stl {
    class Parker {
        EventFD ev_;
        alignas(64) bool sleeping_;

        void parkEnter() noexcept;
        void parkLeave() noexcept;

    public:
        Parker();
        ~Parker() noexcept;

        int fd() const noexcept;

        void drain() noexcept;
        void unpark() noexcept;
        void signal() noexcept;

        template <typename F>
        void park(F f) {
            parkEnter();
            f();
            parkLeave();
        }
    };
}

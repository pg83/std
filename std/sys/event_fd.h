#pragma once

namespace stl {
    class EventFD {
        struct Impl;
        Impl* impl;

    public:
        EventFD();
        ~EventFD() noexcept;

        int fd() const noexcept;

        void signal();
        void drain();
    };
}

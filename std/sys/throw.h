#pragma once

namespace stl {
    class Buffer;
    class StringView;

    enum class ExceptionKind {
        Errno,
    };

    struct Exception {
        virtual ~Exception();

        virtual ExceptionKind kind() const = 0;
        virtual StringView description() = 0;

        static StringView current();
    };

    struct Errno {
        int error;

        Errno();

        [[noreturn]]
        void raise(Buffer&& text);
    };
}

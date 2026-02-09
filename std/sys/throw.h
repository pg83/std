#pragma once

namespace Std {
    class Buffer;
    class StringView;

    enum class ExceptionKind {
        Errno,
    };

    struct Exception {
        virtual ~Exception() noexcept;

        virtual ExceptionKind kind() const noexcept = 0;
        virtual StringView description() = 0;
    };

    struct Errno {
        int error;

        Errno() noexcept;

        [[noreturn]]
        void raise(Buffer&& text);
    };
}

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

    [[noreturn]]
    void throwErrno(int err, Buffer&& text);
}

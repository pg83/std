#pragma once

namespace Std {
    class Buffer;

    enum class ExceptionKind {
        Errno,
    };

    struct Exception {
        virtual ~Exception() noexcept;

        virtual ExceptionKind kind() const noexcept = 0;
    };

    void throwErrno(int err, Buffer&& text);
}

#pragma once

#include <std/sys/types.h>

namespace stl {
    class Buffer;
    class StringView;

    enum class ExceptionKind {
        Errno,
        Verify,
    };

    struct Exception {
        virtual ~Exception() noexcept;

        virtual ExceptionKind kind() const noexcept = 0;
        virtual StringView description() = 0;

        static StringView current();
    };

    [[noreturn]]
    void raiseVerify(const u8* what, u32 line, const u8* file);

    struct Errno {
        int error;

        Errno() noexcept;
        explicit Errno(int error) noexcept;

        [[noreturn]]
        void raise(Buffer&& text);
    };
}

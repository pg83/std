#pragma once

#include <std/os/types.h>

#define stdErr (::Std::stderrStream())
#define stdOut (::Std::stdoutStream())

namespace Std {
    struct Output;

    template <typename T>
    void output(Output& out, const T& t);

    struct Output {
        inline void write(const void* data, size_t len) {
            if (len) {
                writeImpl(data, len);
            }
        }

        inline void write(char ch) {
            write(&ch, 1);
        }

        template <typename Out, typename T>
        friend inline Out& operator<<(Out& out, const T& t) {
            output(out, t);

            return out;
        }

    private:
        virtual void writeImpl(const void* data, size_t len) = 0;
    };

    Output& stdoutStream() noexcept;
    Output& stderrStream() noexcept;
}

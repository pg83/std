#include "sys.h"
#include "output.h"

#include <std/lib/singleton.h>

#include <stdio.h>

using namespace Std;

namespace {
    struct StdOutput: public Output {
        inline StdOutput(FILE* s) noexcept
            : stream(s)
        {
        }

        void writeImpl(const void* data, size_t len) override {
            fwrite(data, 1, len, stream);
        }

        void flushImpl() override {
            fflush(stream);
        }

        FILE* stream;
    };

    struct StdErr: public StdOutput {
        inline StdErr() noexcept
            : StdOutput(stderr)
        {
        }
    };

    struct StdOut: public StdOutput {
        inline StdOut() noexcept
            : StdOutput(stdout)
        {
        }
    };
}

Output& Std::stdoutStream() noexcept {
    return singleton<StdOut>();
}

Output& Std::stderrStream() noexcept {
    return singleton<StdErr>();
}

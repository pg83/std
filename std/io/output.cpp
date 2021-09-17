#include "output.h"

#include <std/tl/singleton.h>

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

Output::~Output() {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

namespace Std {
    Output& stdoutStream() noexcept {
        return singleton<StdOut>();
    }

    Output& stderrStream() noexcept {
        return singleton<StdErr>();
    }

    template <>
    void output<EndLine>(Output& out, const EndLine&) {
        out.write('\n');
        out.flush();
    }

    template <>
    void output<c8>(Output& out, const c8& ch) {
        out.write(&ch, 1);
    }
}

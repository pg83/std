#include "fd.h"
#include "sys.h"
#include "output.h"

#include <std/lib/singleton.h>

using namespace Std;

namespace {
    struct StdErr: public FDOutput {
        inline StdErr() noexcept
            : FDOutput(2)
        {
        }
    };

    struct StdOut: public FDOutput {
        inline StdOut() noexcept
            : FDOutput(1)
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

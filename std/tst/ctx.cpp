#include "ctx.h"

#if __has_include(<execinfo.h>)
    #include <execinfo.h>
#endif

using namespace stl;

void Ctx::printTB() const {
#if __has_include(<execinfo.h>)
    void* frames[64];
    int n = backtrace(frames, 64);

    backtrace_symbols_fd(frames, n, 2);
#endif
}

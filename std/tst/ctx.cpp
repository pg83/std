#include "ctx.h"

#if __has_include(<execinfo.h>)
    #include <execinfo.h>
#else
    extern "C" int backtrace(void**, int);
    extern "C" void backtrace_symbols_fd(void* const*, int, int);
#endif

using namespace stl;

void Ctx::printTB() const {
    void* frames[64];
    int n = backtrace(frames, 64);

    backtrace_symbols_fd(frames, n, 2);
}

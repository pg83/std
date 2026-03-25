#include <std/tst/ut.h>
#include <std/tst/ctx.h>

#if __has_include(<cpptrace/cpptrace.hpp>)
    #undef noexcept
    #include <cpptrace/cpptrace.hpp>
#endif

using namespace stl;

namespace {
    struct MyCtx: public Ctx {
        MyCtx(int c, char** v) {
            argc = c;
            argv = v;
        }

#if __has_include(<cpptrace/cpptrace.hpp>)
        void printTB() const override {
            cpptrace::generate_trace().print();
        }
#endif
    };
}

int main(int argc, char** argv) {
    MyCtx(argc, argv).run();
}

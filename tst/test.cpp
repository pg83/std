#include <std/tst/ut.h>
#include <std/tst/ctx.h>

#include <cpptrace/cpptrace.hpp>

using namespace Std;

namespace {
    struct MyCtx: public Ctx {
        inline MyCtx(int c, char** v) {
            argc = c;
            argv = v;
        }

        void printTB() const override {
            cpptrace::generate_trace().print();
        }
    };
}

int main(int argc, char** argv) {
    MyCtx(argc, argv).run();
}

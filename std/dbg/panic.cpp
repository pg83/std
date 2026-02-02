#include "panic.h"
#include "color.h"

#include <std/ios/sys.h>
#include <std/sys/crt.h>
#include <std/str/view.h>
#include <std/ios/output.h>
#include <std/alg/exchange.h>

#include <stdlib.h>

using namespace Std;

namespace {
    static void emptyFunc() {
    }

    static PanicHandler panicHandler1 = (PanicHandler)emptyFunc;
    static PanicHandler panicHandler2 = (PanicHandler)abort;
}

PanicHandler Std::setPanicHandler1(PanicHandler hndl) noexcept {
    return exchange(panicHandler1, hndl);
}

PanicHandler Std::setPanicHandler2(PanicHandler hndl) noexcept {
    return exchange(panicHandler2, hndl);
}

void Std::panic(const u8* what, u32 line, const u8* file) {
    panicHandler1();

    sysE << Color::bright(AnsiColor::Red)
         << StringView(what, strLen(what))
         << StringView(u8" failed, at ")
         << StringView(file, strLen(file))
         << StringView(u8":")
         << line << Color::reset() << endL << finI;

    panicHandler2();
}

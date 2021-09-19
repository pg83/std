#include "panic.h"
#include "color.h"

#include <std/str/view.h>

#include <std/ios/sys.h>
#include <std/ios/output.h>

#include <stdlib.h>

void Std::panic(const u8* what, u32 line, const u8* file) {
    sysE << endL << Color::bright(AnsiColor::Red)
         << what << StringView(u8" failed, at ")
         << file << StringView(u8":")
         << line << Color::reset() << endL << finI;

    abort();
}

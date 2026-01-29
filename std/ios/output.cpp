#include "output.h"

#include <std/str/view.h>
#include <std/alg/range.h>

using namespace Std;

Output::~Output() noexcept {
}

void Output::flushImpl() {
}

void Output::finishImpl() {
}

void Output::writeVImpl(const StringView* parts, size_t count) {
    for (const auto& it : range(parts, parts + count)) {
        write(it.data(), it.length());
    }
}

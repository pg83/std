#include <std/lib/range.h>
#include <std/lib/buffer.h>
#include <std/lib/vector.h>
#include <std/ios/output.h>
#include <std/str/dynamic.h>

using namespace Std;

int main() {
    Vector<u64> v;

    for (size_t i = 0; i < 100; ++i) {
        v.pushBack(i);
    }

    for (auto x : range(v)) {
        stdE << x << endL;
    }
}

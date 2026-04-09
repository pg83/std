#include <std/sys/types.h>

namespace {
    void doW(int work) {
        for (volatile int i = 0; i < work; i = i + 1) {
        }
    }
}

int main() {
    for (volatile int i = 0; i < 10000000; i = i + 1) {
        doW(250);
    }
}

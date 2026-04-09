#include <stdlib.h>
#include <std/sys/types.h>

int main() {
    volatile size_t res = 0;

    for (size_t i = 0; i < 100000000; ++i) {
        auto p = malloc(256);
        res += (size_t)p;
        free(p);
    }
}

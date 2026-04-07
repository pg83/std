#!/bin/sh

set -xue

EXTRA=
#EXTRA=lib/build/opt/O0

~/ix/ix run \
    set/dev/cc \
    bld/make \
    bld/sh \
    bld/box \
    ${EXTRA} \
    lib/c++ \
    lib/kernel \
    lib/aws/lc \
    lib/c/ares \
    lib/xxhash \
    ${EXTRA} \
    -- ./dev/build.sh

time timeout 10s ./tst/test --top=20

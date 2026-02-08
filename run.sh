#!/bin/sh

set -xue

~/ix/ix run set/dev/cc bld/make bld/sh bld/box lib/build/opt/O0 lib/c++ lib/rapidhash lib/cpp/trace lib/build/opt/O0 -- ./build.sh
time ./tst/test

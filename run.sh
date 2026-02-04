#!/bin/sh

set -xue

~/ix/ix run set/dev/cc bld/make bld/sh bld/box lib/c++ lib/rapidhash lib/cpp/trace -- ./build.sh
time ./tst/test

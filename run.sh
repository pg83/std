#!/bin/sh

set -xue

~/ix/ix run set/dev/cc bld/make bld/sh bld/box lib/c lib/c++ lib/dwarf lib/zstd lib/rapidhash lib/cpp/trace -- make
time ./tst/test

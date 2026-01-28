#!/bin/sh

set -xue

~/ix/ix run set/dev/cc bld/make bld/sh bld/box lib/c lib/rapidhash -- make
./tst/test

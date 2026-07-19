#!/bin/sh

set -xue

CPPFLAGS="${CPPFLAGS} -DENABLE_ASSERT=1" ./build -j 48 > /dev/null

#!/bin/sh

set -xue

(make -j 48 LDFLAGS="-fuse-ld=lld ${LDFLAGS}" EXTRA=-DENABLE_ASSERT=1 || make LDFLAGS="-fuse-ld=lld ${LDFLAGS}" EXTRA=-DENABLE_ASSERT=1) > /dev/null

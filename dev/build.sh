#!/bin/sh

set -xue

(make -j 48 LDFLAGS="${LDFLAGS}" EXTRA=-DENABLE_ASSERT=1 || make LDFLAGS="${LDFLAGS}" EXTRA=-DENABLE_ASSERT=1) > /dev/null

#!/bin/sh

set -xue

(make -j 24 EXTRA=-DENABLE_ASSERT=1 || make EXTRA=-DENABLE_ASSERT=1) > /dev/null

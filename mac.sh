#!/bin/sh

set -xue

COMMON="-fsanitize=address"

make CXX=clang++ \
    CPPFLAGS="-D_XOPEN_SOURCE -isystem/Library/Developer/CommandLineTools/SDKs/MacOSX12.3.sdk/usr/include ${COMMON}" \
    LDFLAGS="-L/Library/Developer/CommandLineTools/SDKs/MacOSX12.3.sdk/usr/lib ${COMMON}" \
    -j 16

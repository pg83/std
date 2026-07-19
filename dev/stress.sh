#!/bin/sh

set -xue

while true; do
    time ./.build/tst/test "${@}" > /dev/null
done

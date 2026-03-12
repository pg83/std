#!/bin/sh

set -xue

while true; do
    time timeout 10s ./tst/test > /dev/null
done

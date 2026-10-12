#!/bin/bash
set -e
cd ~/shivrai-hon-core
T=${1:?usage: tools/test.sh <test_name>}
# test_stablecoin → stablecoin
M=${T#test_}
g++ -std=c++17 -Wall -Wextra -O2 -Isrc \
  tests/shivrai/${T}.cpp \
  src/shivrai/${M}/${M}.cpp \
  -o /tmp/${T}
/tmp/${T}

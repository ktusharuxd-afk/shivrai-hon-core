#!/bin/bash
set -e
M=${1:?usage: tools/new_module.sh <module>}
D=$HOME/shivrai-hon-core
UP=$(echo "$M" | tr '[:lower:]' '[:upper:]')
mkdir -p "$D/src/shivrai/$M" "$D/tests/shivrai"

cat > "$D/src/shivrai/$M/${M}.h" << H
#ifndef SHIVRAI_HON_${UP}_H
#define SHIVRAI_HON_${UP}_H
namespace shivrai::${M} {
class ${M} {
public:
    ${M}() = default;
};
}
#endif
H

cat > "$D/src/shivrai/$M/${M}.cpp" << C
#include "${M}.h"
namespace shivrai::${M} {}
C

cat > "$D/tests/shivrai/test_${M}.cpp" << T
#include "shivrai/${M}/${M}.h"
#include <cassert>
#include <iostream>
using namespace shivrai::${M};
int main() {
    int passed = 0;
    (void)passed;
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
T

echo "OK Module '$M' created:"
echo "   src/shivrai/$M/${M}.h"
echo "   src/shivrai/$M/${M}.cpp"
echo "   tests/shivrai/test_${M}.cpp"

# Contributing

## Quick Start
    git clone https://github.com/ktusharuxd-afk/shivrai-hon-core.git
    cd shivrai-hon-core
    bash scripts/build.sh
    bash scripts/run_node.sh

## Adding a module
1. include/shivrai/<name>.hpp — declare register_<name>(RpcServer&, State&)
2. src/<name>.cpp — implement
3. Wire into src/main.cpp
4. Add tests in tests/test_ext.cpp
5. Document in README.md

## Pull Requests
1. Fork, feature branch, add tests, ensure build passes, open PR.

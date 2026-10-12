#pragma once
#include "shivrai/rpc.hpp"
#include "shivrai/state.hpp"
#include "shivrai/contract.hpp"
namespace shivrai {
void register_contract(RpcServer&, State&);
}

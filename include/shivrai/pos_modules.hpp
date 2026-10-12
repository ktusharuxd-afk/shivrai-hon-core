#pragma once
#include "shivrai/rpc.hpp"
#include "shivrai/state.hpp"
#include "shivrai/pos.hpp"
namespace shivrai {
void register_pos(RpcServer&, State&, PoSConsensus&);
}

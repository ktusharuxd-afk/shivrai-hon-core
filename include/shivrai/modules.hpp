#pragma once
#include "shivrai/rpc.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/mempool.hpp"
namespace shivrai {
void register_chain(RpcServer&, Blockchain&, Mempool&, State&);
void register_stablecoin(RpcServer&, State&);
void register_lending(RpcServer&, State&);
void register_governance(RpcServer&, State&);
void register_token(RpcServer&, State&);
}

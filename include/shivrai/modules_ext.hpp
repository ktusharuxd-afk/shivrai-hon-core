#pragma once
#include "shivrai/rpc.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
namespace shivrai {
void register_nft(RpcServer&, State&);
void register_amm(RpcServer&, State&);
void register_staking(RpcServer&, State&, Blockchain&);
void register_multisig(RpcServer&, State&);
class P2P;
void register_p2p(RpcServer&, P2P&);
void register_explorer(RpcServer&, Blockchain&, State&);
}

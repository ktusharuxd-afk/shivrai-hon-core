#pragma once
#include <memory>
namespace shivrai {
class RpcServer;
class Blockchain;
class State;
void register_rest(RpcServer&, Blockchain&, State&);
}

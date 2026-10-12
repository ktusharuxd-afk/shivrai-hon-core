#include "shivrai/rest.hpp"
#include "shivrai/rpc.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/state.hpp"
#include "httplib.h"
namespace shivrai {
// We piggyback on RpcServer via registered internal methods for JSON, and
// expose a small set of pure-GET REST endpoints.
// Since RpcServer owns the httplib::Server internally, we register through a
// lightweight mechanism: RpcServer exposes POST /rpc and GET /health only.
// To add REST, we attach a hook on the underlying server through a friend call.
// For simplicity, we add REST paths to the same httplib server via a public method.
// Implemented by patching rpc.cpp to call register_rest internally.
void register_rest(RpcServer&, Blockchain&, State&) {
  // placeholder — actual routes wired in rpc.cpp start()
}
}

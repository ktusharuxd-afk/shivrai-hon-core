#include "shivrai/storage.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/mempool.hpp"
#include "shivrai/wallet.hpp"
#include "shivrai/rpc.hpp"
#include "shivrai/modules.hpp"
#include "shivrai/modules_ext.hpp"
#include "shivrai/consensus.hpp"
#include "shivrai/p2p.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstring>

using namespace shivrai;

static std::atomic<bool> g_run{true};
static void on_sig(int) { g_run = false; }

int main(int argc, char** argv) {
  std::string db   = "shivrai.db";
  std::string host = "0.0.0.0";
  int         port = 8545;
  uint64_t    iv   = 3000;
  bool        novalidate = false;
  std::vector<std::string> peer_urls;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if      (a == "--db"   && i+1 < argc) db   = argv[++i];
    else if (a == "--port" && i+1 < argc) port = std::stoi(argv[++i]);
    else if (a == "--host" && i+1 < argc) host = argv[++i];
    else if (a == "--interval" && i+1 < argc) iv = std::stoull(argv[++i]);
    else if (a == "--peer" && i+1 < argc) peer_urls.push_back(argv[++i]);
    else if (a == "--novalidate") novalidate = true;
    else if (a == "--help") {
      std::cout << "Usage: shivrai-node [--db path] [--host h] [--port p]\n"
                   "                     [--interval ms] [--peer URL]... [--novalidate]\n";
      return 0;
    }
  }

  signal(SIGINT,  on_sig);
  signal(SIGTERM, on_sig);

  std::cout << ">>> shivrai-hon-core v1.0.0\n";
  std::cout << ">>> db="   << db
            << " rpc="  << host << ":" << port
            << " interval=" << iv << "ms"
            << " mode=" << (novalidate ? "FOLLOWER" : "VALIDATOR") << "\n";

  auto storage = std::make_shared<Storage>(db);
  storage->init_schema();

  State      state(storage);
  Blockchain chain(storage);
  chain.load();
  Mempool    pool;
  Wallet     wallet;

  std::cout << ">>> validator: " << wallet.address() << "\n";
  std::cout << ">>> height: "    << chain.height() << "\n";

  if (!novalidate && chain.height() == 0 && state.balance(wallet.address()) == 0)
    state.mint(wallet.address(), 1000000000ULL);

  RpcServer rpc;
  P2P       p2p(chain);

  for (auto& u : peer_urls) p2p.add_peer(u);

  rpc.set_p2p_receiver([&](const json& bj, std::string& r) {
    return p2p.receive_block(bj.get<Block>(), r);
  });

  register_p2p(rpc, p2p);
  register_chain(rpc, chain, pool, state);
  register_stablecoin(rpc, state);
  register_lending(rpc, state);
  register_governance(rpc, state);
  register_token(rpc, state);
  register_nft(rpc, state);
  register_amm(rpc, state);
  register_staking(rpc, state, chain);
  register_multisig(rpc, state);
  register_explorer(rpc, chain, state);

  rpc.start(host, port);
  std::cout << ">>> RPC ready: http://" << host << ":" << port << "/rpc\n";

  // Sync from all peers at startup (both modes)
  auto peers = p2p.peers();
  for (auto& u : peers) {
    std::cout << ">>> syncing from " << u << "...\n";
    p2p.sync_from_peer(u);
  }

  PoAConsensus consensus(chain, pool, state, wallet, iv);
  if (!novalidate) {
    consensus.set_block_callback([&](const Block& b) { p2p.broadcast_block(b); });
    consensus.start();
    std::cout << ">>> consensus started (producing blocks)\n";
  } else {
    std::cout << ">>> follower mode (receive-only)\n";
  }

  while (g_run) std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "\n>>> shutdown, height=" << chain.height() << "\n";
  consensus.stop();
  rpc.stop();
  return 0;
}

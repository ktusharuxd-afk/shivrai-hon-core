#include "shivrai/storage.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/mempool.hpp"
#include "shivrai/wallet.hpp"
#include "shivrai/rpc.hpp"
#include "shivrai/modules.hpp"
#include "shivrai/modules_ext.hpp"
#include "shivrai/consensus.hpp"
#include "shivrai/pos.hpp"
#include "shivrai/pos_modules.hpp"
#include "shivrai/p2p.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <memory>

using namespace shivrai;

static std::atomic<bool> g_run{true};
static void on_sig(int) { g_run = false; }

int main(int argc, char** argv) {
  std::string db = "shivrai.db";
  std::string host = "0.0.0.0";
  int port = 8545;
  uint64_t iv = 3000;
  bool novalidate = false;
  std::string consensus_kind = "poa";
  std::vector<std::string> peer_urls;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if      (a == "--db" && i+1 < argc) db = argv[++i];
    else if (a == "--port" && i+1 < argc) port = std::stoi(argv[++i]);
    else if (a == "--host" && i+1 < argc) host = argv[++i];
    else if (a == "--interval" && i+1 < argc) iv = std::stoull(argv[++i]);
    else if (a == "--peer" && i+1 < argc) peer_urls.push_back(argv[++i]);
    else if (a == "--novalidate") novalidate = true;
    else if (a == "--consensus" && i+1 < argc) consensus_kind = argv[++i];
    else if (a == "--help") {
      std::cout << "Usage: shivrai-node [opts]\n"
                   "  --db PATH | --host H | --port N | --interval MS\n"
                   "  --peer URL (repeatable)\n"
                   "  --consensus poa|pos  (default poa)\n"
                   "  --novalidate         (follower)\n";
      return 0;
    }
  }

  signal(SIGINT, on_sig);
  signal(SIGTERM, on_sig);

  std::cout << ">>> shivrai-hon-core v1.0.0\n";
  std::cout << ">>> db=" << db << " rpc=" << host << ":" << port
            << " interval=" << iv << "ms"
            << " consensus=" << consensus_kind
            << " mode=" << (novalidate ? "FOLLOWER" : "VALIDATOR") << "\n";

  auto storage = std::make_shared<Storage>(db);
  storage->init_schema();

  State state(storage);
  Blockchain chain(storage);
  chain.load();
  Mempool pool;
  Wallet  wallet;

  std::cout << ">>> validator: " << wallet.address() << "\n";
  std::cout << ">>> height: "    << chain.height() << "\n";

  if (!novalidate && chain.height() == 0 && state.balance(wallet.address()) == 0)
    state.mint(wallet.address(), 1'000'000'000ULL);

  if (!novalidate && consensus_kind == "pos") {
    if (get_validator(state, wallet.address()).pubkey.empty()) {
      uint64_t init_stake = 1000;
      if (state.balance(wallet.address()) >= init_stake) {
        register_validator(state, wallet.address(),
                           wallet.keypair().public_key_hex, init_stake);
        std::cout << ">>> auto-registered as PoS validator (stake="
                  << init_stake << ")\n";
      }
    }
  }

  RpcServer rpc;
  P2P p2p(chain);
  for (auto& u : peer_urls) p2p.add_peer(u);
  rpc.set_p2p_receiver([&](const json& bj, std::string& r) {
    return p2p.receive_block(bj.get<Block>(), r);
  });

  std::unique_ptr<PoAConsensus> poa;
  std::unique_ptr<PoSConsensus> pos;
  if (consensus_kind == "pos")
    pos.reset(new PoSConsensus(chain, pool, state, wallet, iv));

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
  if (pos) register_pos(rpc, state, *pos);

  rpc.start(host, port);
  std::cout << ">>> RPC ready: http://" << host << ":" << port << "/rpc\n";

  for (auto& u : p2p.peers()) p2p.sync_from_peer(u);

  if (!novalidate) {
    if (pos) {
      pos->set_block_callback([&](const Block& b){ p2p.broadcast_block(b); });
      pos->start();
      std::cout << ">>> PoS consensus started\n";
    } else {
      poa.reset(new PoAConsensus(chain, pool, state, wallet, iv));
      poa->set_block_callback([&](const Block& b){ p2p.broadcast_block(b); });
      poa->start();
      std::cout << ">>> PoA consensus started\n";
    }
  } else {
    std::cout << ">>> follower mode\n";
  }

  while (g_run) std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "\n>>> shutdown, height=" << chain.height() << "\n";
  if (poa) poa->stop();
  if (pos) pos->stop();
  rpc.stop();
  return 0;
}

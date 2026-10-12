#include "shivrai/crypto.hpp"
#include "shivrai/wallet.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/rpc.hpp"
#include "shivrai/modules.hpp"
#include <chrono>
#include <iostream>
#include <cstdio>
#include <memory>
using namespace shivrai;
using clk = std::chrono::high_resolution_clock;
static double ms(clk::time_point a, clk::time_point b) {
  return std::chrono::duration<double, std::milli>(b-a).count();
}
int main() {
  std::remove("/tmp/bench.db");
  auto st = std::make_shared<Storage>("/tmp/bench.db");
  st->init_schema();
  State state(st); Blockchain chain(st); Mempool pool;

  const int N = 1000;
  // --- ECDSA keygen ---
  auto t0 = clk::now();
  for (int i = 0; i < N; ++i) { auto kp = crypto::generate_keypair(); (void)kp; }
  auto t1 = clk::now();
  printf("keygen:      %6.2f ms  (%d keys, %.0f/s)\n",
         ms(t0,t1), N, N/(ms(t0,t1)/1000.0));

  // --- sign ---
  auto kp = crypto::generate_keypair();
  t0 = clk::now();
  for (int i = 0; i < N; ++i) crypto::sign(kp.private_key_hex, "msg" + std::to_string(i));
  t1 = clk::now();
  printf("sign:        %6.2f ms  (%d sigs, %.0f/s)\n",
         ms(t0,t1), N, N/(ms(t0,t1)/1000.0));

  // --- verify ---
  std::vector<std::string> sigs, msgs;
  for (int i = 0; i < N; ++i) {
    auto m = "msg" + std::to_string(i);
    sigs.push_back(crypto::sign(kp.private_key_hex, m));
    msgs.push_back(m);
  }
  t0 = clk::now();
  int ok = 0;
  for (int i = 0; i < N; ++i)
    if (crypto::verify(kp.public_key_hex, msgs[i], sigs[i])) ++ok;
  t1 = clk::now();
  printf("verify:      %6.2f ms  (%d/%d ok, %.0f/s)\n",
         ms(t0,t1), ok, N, N/(ms(t0,t1)/1000.0));

  // --- sha256 ---
  t0 = clk::now();
  for (int i = 0; i < N*10; ++i) crypto::sha256_hex("data" + std::to_string(i));
  t1 = clk::now();
  printf("sha256:      %6.2f ms  (%d hashes, %.0f/s)\n",
         ms(t0,t1), N*10, (N*10)/(ms(t0,t1)/1000.0));

  // --- state write ---
  t0 = clk::now();
  for (int i = 0; i < N; ++i) state.set_u64("bench:" + std::to_string(i), i);
  t1 = clk::now();
  printf("state.put:   %6.2f ms  (%d writes, %.0f/s)\n",
         ms(t0,t1), N, N/(ms(t0,t1)/1000.0));

  // --- state read ---
  t0 = clk::now();
  uint64_t sum = 0;
  for (int i = 0; i < N; ++i) sum += state.get_u64("bench:" + std::to_string(i));
  t1 = clk::now();
  printf("state.get:   %6.2f ms  (%d reads, %.0f/s, sum=%llu)\n",
         ms(t0,t1), N, N/(ms(t0,t1)/1000.0), (unsigned long long)sum);

  // --- block with 100 txs ---
  std::vector<Transaction> txs;
  for (int i = 0; i < 100; ++i)
    txs.push_back(kp.address.empty() ? Transaction{} : [&]{
      Wallet w(kp);
      return w.create_tx("0xdead", 1, i, "transfer");
    }());
  t0 = clk::now();
  chain.add_block(txs, kp.address, state.state_root());
  t1 = clk::now();
  printf("block(100tx):%6.2f ms\n", ms(t0,t1));

  // --- RPC dispatch (in-process) ---
  RpcServer rpc;
  register_chain(rpc, chain, pool, state);
  register_stablecoin(rpc, state);
  t0 = clk::now();
  for (int i = 0; i < N; ++i) {
    rpc.handle_request({{"jsonrpc","2.0"},{"id",i},{"method","chain_info"},{"params",{}}});
  }
  t1 = clk::now();
  printf("rpc.dispatch:%6.2f ms  (%d calls, %.0f/s)\n",
         ms(t0,t1), N, N/(ms(t0,t1)/1000.0));

  return 0;
}

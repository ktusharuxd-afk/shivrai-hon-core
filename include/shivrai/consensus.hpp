#pragma once
#include "shivrai/blockchain.hpp"
#include "shivrai/mempool.hpp"
#include "shivrai/state.hpp"
#include "shivrai/wallet.hpp"
#include <atomic>
#include <thread>
#include <functional>
namespace shivrai {
class PoAConsensus {
public:
  PoAConsensus(Blockchain&, Mempool&, State&, Wallet&, uint64_t interval_ms=3000);
  ~PoAConsensus();
  void start();
  void stop();
  using BlockCB = std::function<void(const Block&)>;
  void set_block_callback(BlockCB cb) { on_block_ = std::move(cb); }
private:
  void loop();
  Blockchain& chain_; Mempool& pool_; State& state_; Wallet& wallet_;
  uint64_t interval_ms_;
  std::atomic<bool> running_{false};
  std::thread thr_;
  BlockCB on_block_;
};
}

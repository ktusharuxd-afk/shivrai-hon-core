#pragma once
#include "shivrai/blockchain.hpp"
#include "shivrai/mempool.hpp"
#include "shivrai/state.hpp"
#include "shivrai/wallet.hpp"
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace shivrai {

struct Validator {
  std::string address;
  std::string pubkey;
  uint64_t stake = 0;
  uint64_t blocks_produced = 0;
  bool active = true;
};

// ---- Registry ----
std::vector<Validator> list_validators(State&);
Validator get_validator(State&, const std::string& addr);
void register_validator(State&, const std::string& addr,
                        const std::string& pubkey, uint64_t stake);
void add_stake(State&, const std::string& addr, uint64_t amount);
bool remove_stake(State&, const std::string& addr, uint64_t amount);
uint64_t total_stake(State&);
void slash_validator(State&, const std::string& addr, uint64_t amount);

// ---- Consensus ----
class PoSConsensus {
public:
  PoSConsensus(Blockchain&, Mempool&, State&, Wallet&, uint64_t interval_ms=3000);
  ~PoSConsensus();
  void start();
  void stop();
  using BlockCB = std::function<void(const Block&)>;
  void set_block_callback(BlockCB cb) { on_block_ = std::move(cb); }
  std::string select_leader(uint64_t height);
private:
  void loop();
  Blockchain& chain_;
  Mempool& pool_;
  State& state_;
  Wallet& wallet_;
  uint64_t interval_ms_;
  std::atomic<bool> running_{false};
  std::thread thr_;
  BlockCB on_block_;
};

}

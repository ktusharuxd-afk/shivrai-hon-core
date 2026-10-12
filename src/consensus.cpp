#include "shivrai/consensus.hpp"
#include <chrono>
#include <cstdio>
namespace shivrai {
PoAConsensus::PoAConsensus(Blockchain& c, Mempool& p, State& s, Wallet& w, uint64_t iv)
  : chain_(c), pool_(p), state_(s), wallet_(w), interval_ms_(iv) {}
PoAConsensus::~PoAConsensus() { stop(); }
void PoAConsensus::start() { running_=true; thr_=std::thread([this]{loop();}); }
void PoAConsensus::stop() { running_=false; if (thr_.joinable()) thr_.join(); }
void PoAConsensus::loop() {
  while (running_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    auto txs = pool_.drain(100);
    for (auto& t : txs) state_.apply_tx(t);
    if (!txs.empty() || chain_.height()==0) {
      auto b = chain_.add_block(txs, wallet_.address(), state_.state_root());
      if (on_block_) on_block_(b);
      fprintf(stderr, "[consensus] block %llu txs=%zu\n",
        (unsigned long long)b.height, txs.size());
    }
  }
}
}

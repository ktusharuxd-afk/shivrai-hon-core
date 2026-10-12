#include "shivrai/pos.hpp"
#include "shivrai/crypto.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdexcept>

namespace shivrai {

static std::string vkey(const std::string& addr, const std::string& f) {
  return "validator:" + addr + ":" + f;
}

static std::vector<std::string> validator_addrs(State& st) {
  std::string csv = st.get_str("validator:addrs", "");
  std::vector<std::string> out;
  size_t p = 0;
  while (p < csv.size()) {
    size_t c = csv.find(',', p);
    if (c == std::string::npos) c = csv.size();
    if (c > p) out.push_back(csv.substr(p, c - p));
    p = c + 1;
  }
  return out;
}

static void add_validator_addr(State& st, const std::string& addr) {
  auto list = validator_addrs(st);
  for (auto& a : list) if (a == addr) return;
  list.push_back(addr);
  std::string csv;
  for (size_t i = 0; i < list.size(); ++i) {
    if (i) csv += ",";
    csv += list[i];
  }
  st.set_str("validator:addrs", csv);
}

Validator get_validator(State& st, const std::string& addr) {
  Validator v;
  v.address = addr;
  v.pubkey  = st.get_str(vkey(addr, "pubkey"), "");
  v.stake   = st.get_u64(vkey(addr, "stake"), 0);
  v.blocks_produced = st.get_u64(vkey(addr, "blocks"), 0);
  v.active  = st.get_u64(vkey(addr, "active"), v.pubkey.empty() ? 0 : 1) != 0;
  return v;
}

std::vector<Validator> list_validators(State& st) {
  std::vector<Validator> out;
  for (auto& a : validator_addrs(st)) out.push_back(get_validator(st, a));
  return out;
}

void register_validator(State& st, const std::string& addr,
                        const std::string& pubkey, uint64_t stake) {
  if (addr.empty() || pubkey.empty())
    throw std::runtime_error("addr and pubkey required");
  if (!get_validator(st, addr).pubkey.empty())
    throw std::runtime_error("already registered");
  if (st.balance(addr) < stake)
    throw std::runtime_error("insufficient balance for stake");
  st.set_balance(addr, st.balance(addr) - stake);
  st.set_str(vkey(addr, "pubkey"), pubkey);
  st.set_u64(vkey(addr, "stake"), stake);
  st.set_u64(vkey(addr, "blocks"), 0);
  st.set_u64(vkey(addr, "active"), 1);
  add_validator_addr(st, addr);
}

void add_stake(State& st, const std::string& addr, uint64_t amount) {
  auto v = get_validator(st, addr);
  if (v.pubkey.empty()) throw std::runtime_error("not a validator");
  if (st.balance(addr) < amount) throw std::runtime_error("insufficient balance");
  st.set_balance(addr, st.balance(addr) - amount);
  st.set_u64(vkey(addr, "stake"), v.stake + amount);
}

bool remove_stake(State& st, const std::string& addr, uint64_t amount) {
  auto v = get_validator(st, addr);
  if (v.pubkey.empty()) return false;
  if (v.stake < amount) return false;
  st.set_u64(vkey(addr, "stake"), v.stake - amount);
  st.set_balance(addr, st.balance(addr) + amount);
  return true;
}

uint64_t total_stake(State& st) {
  uint64_t t = 0;
  for (auto& v : list_validators(st)) if (v.active) t += v.stake;
  return t;
}

void slash_validator(State& st, const std::string& addr, uint64_t amount) {
  auto v = get_validator(st, addr);
  if (v.pubkey.empty()) return;
  uint64_t cut = std::min(amount, v.stake);
  st.set_u64(vkey(addr, "stake"), v.stake - cut);
  if (v.stake - cut == 0) st.set_u64(vkey(addr, "active"), 0);
}

PoSConsensus::PoSConsensus(Blockchain& c, Mempool& p, State& s, Wallet& w, uint64_t iv)
  : chain_(c), pool_(p), state_(s), wallet_(w), interval_ms_(iv) {}
PoSConsensus::~PoSConsensus() { stop(); }

void PoSConsensus::start() { running_ = true; thr_ = std::thread([this]{ loop(); }); }
void PoSConsensus::stop()  { running_ = false; if (thr_.joinable()) thr_.join(); }

std::string PoSConsensus::select_leader(uint64_t height) {
  auto vals = list_validators(state_);
  uint64_t total = 0;
  for (auto& v : vals) if (v.active && v.stake > 0) total += v.stake;
  if (total == 0) return wallet_.address();

  std::string seed = chain_.latest().hash + "|" + std::to_string(height);
  std::string h = crypto::sha256_hex(seed);
  uint64_t target = std::stoull(h.substr(0, 16), nullptr, 16) % total;

  uint64_t acc = 0;
  std::string last;
  for (auto& v : vals) {
    if (!v.active || v.stake == 0) continue;
    acc += v.stake;
    last = v.address;
    if (target < acc) return v.address;
  }
  return last;
}

void PoSConsensus::loop() {
  while (running_) {
    std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms_));
    uint64_t next_h = chain_.height() + 1;
    std::string leader = select_leader(next_h);
    if (leader != wallet_.address()) continue;

    auto txs = pool_.drain(100);
    for (auto& t : txs) state_.apply_tx(t);
    auto b = chain_.add_block(txs, wallet_.address(), state_.state_root());

    uint64_t bc = state_.get_u64(vkey(wallet_.address(), "blocks"), 0);
    state_.set_u64(vkey(wallet_.address(), "blocks"), bc + 1);

    fprintf(stderr, "[pos] block %llu proposed by %s\n",
            (unsigned long long)b.height, wallet_.address().c_str());
    if (on_block_) on_block_(b);
  }
}

}

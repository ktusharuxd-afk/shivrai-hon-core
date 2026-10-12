#include "shivrai/state.hpp"
#include "shivrai/crypto.hpp"
namespace shivrai {
State::State(std::shared_ptr<Storage> s) : storage_(std::move(s)) {}
uint64_t State::get_u64(const std::string& k, uint64_t d) {
  auto v=storage_->get("state",k); if (!v) return d;
  try { return std::stoull(*v); } catch(...) { return d; }
}
void State::set_u64(const std::string& k, uint64_t v) { storage_->put("state",k,std::to_string(v)); }
std::string State::get_str(const std::string& k, const std::string& d) {
  auto v=storage_->get("state",k); return v?*v:d;
}
void State::set_str(const std::string& k, const std::string& v) { storage_->put("state",k,v); }
uint64_t State::balance(const std::string& a) { return get_u64("bal:"+a,0); }
void State::set_balance(const std::string& a, uint64_t v) { set_u64("bal:"+a,v); }
void State::mint(const std::string& a, uint64_t amt) {
  std::lock_guard<std::mutex> l(mu_);
  set_balance(a, balance(a)+amt);
}
void State::apply_tx(const Transaction& tx) {
  std::lock_guard<std::mutex> l(mu_);
  if (tx.method=="transfer" || tx.method.empty()) {
    uint64_t b=balance(tx.from); if (b<tx.value) return;
    set_balance(tx.from,b-tx.value); set_balance(tx.to,balance(tx.to)+tx.value);
  } else if (tx.method=="stablecoin_mint") {
    uint64_t a=tx.params.value("amount",0ULL);
    set_u64("stable:"+tx.to, get_u64("stable:"+tx.to)+a);
  } else if (tx.method=="lending_deposit") {
    uint64_t a=tx.params.value("amount",0ULL);
    set_u64("lend:"+tx.from, get_u64("lend:"+tx.from)+a);
  } else if (tx.method=="governance_vote") {
    std::string p=tx.params.value("proposal","");
    set_u64("vote:"+p, get_u64("vote:"+p)+1);
  }
}
void State::apply_block(const Block& b) {
  for (auto& t: b.transactions) apply_tx(t);
  set_u64("height", b.height);
}
std::string State::state_root() {
  std::string s = std::to_string(get_u64("height")) + get_str("stable:root","");
  return crypto::sha256_hex(s);
}
}

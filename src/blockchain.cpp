#include "shivrai/blockchain.hpp"
#include "shivrai/crypto.hpp"
#include <chrono>
#include <stdexcept>
namespace shivrai {
static uint64_t now_ms() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
Blockchain::Blockchain(std::shared_ptr<Storage> s) : storage_(std::move(s)) {
  latest_.height=0; latest_.hash="genesis"; latest_.timestamp=now_ms();
  latest_.proposer="genesis"; latest_.state_root="genesis";
}
std::string Blockchain::compute_tx_hash(const Transaction& t) const {
  return crypto::sha256_hex(t.from+t.to+std::to_string(t.value)+std::to_string(t.nonce)
    +std::to_string(t.timestamp)+t.method+t.params.dump());
}
std::string Blockchain::compute_block_hash(const Block& b) const {
  std::string s = std::to_string(b.height)+b.parent_hash+std::to_string(b.timestamp)
    +b.proposer+b.state_root;
  for (auto& t: b.transactions) s += t.hash;
  return crypto::sha256_hex(s);
}
Block Blockchain::add_block(const std::vector<Transaction>& txs,
                            const std::string& prop, const std::string& root) {
  std::lock_guard<std::mutex> l(mu_);
  Block b;
  b.height=latest_.height+1; b.parent_hash=latest_.hash; b.timestamp=now_ms();
  b.proposer=prop; b.transactions=txs; b.state_root=root;
  b.hash=compute_block_hash(b);
  latest_=b;
  storage_->begin_batch();
  try {
    storage_->put("blocks", std::to_string(b.height), json(b).dump());
    storage_->put("state", "height", std::to_string(b.height));
    storage_->put("state", "latest_hash", b.hash);
    storage_->commit_batch();
  } catch (...) { storage_->rollback_batch(); throw; }
  return b;
}
void Blockchain::import_block(const Block& b) {
  std::lock_guard<std::mutex> l(mu_);
  latest_ = b;  // exact copy — hash preserved
  storage_->begin_batch();
  try {
    storage_->put("blocks", std::to_string(b.height), json(b).dump());
    storage_->put("state", "height", std::to_string(b.height));
    storage_->put("state", "latest_hash", b.hash);
    storage_->commit_batch();
  } catch (...) { storage_->rollback_batch(); throw; }
}

Block Blockchain::latest() const { std::lock_guard<std::mutex> l(mu_); return latest_; }
uint64_t Blockchain::height() const { std::lock_guard<std::mutex> l(mu_); return latest_.height; }
Block Blockchain::get_block(uint64_t h) const {
  auto v = storage_->get("blocks", std::to_string(h));
  if (!v) throw std::runtime_error("block not found");
  return json::parse(*v).get<Block>();
}
void Blockchain::load() {
  auto h = storage_->get("state","height"); if (!h) return;
  auto v = storage_->get("blocks", *h); if (v) latest_ = json::parse(*v).get<Block>();
}
}

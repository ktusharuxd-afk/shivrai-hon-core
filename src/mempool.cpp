#include "shivrai/mempool.hpp"
#include <algorithm>
namespace shivrai {
void Mempool::add(const Transaction& t) { std::lock_guard<std::mutex> l(mu_); txs_.push_back(t); }
std::vector<Transaction> Mempool::drain(size_t max) {
  std::lock_guard<std::mutex> l(mu_);
  std::vector<Transaction> o;
  size_t n=std::min(max, txs_.size());
  o.assign(txs_.begin(), txs_.begin()+n);
  txs_.erase(txs_.begin(), txs_.begin()+n);
  return o;
}
size_t Mempool::size() const { std::lock_guard<std::mutex> l(mu_); return txs_.size(); }
}

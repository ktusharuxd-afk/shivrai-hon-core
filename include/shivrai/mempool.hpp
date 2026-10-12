#pragma once
#include "shivrai/types.hpp"
#include <vector>
#include <mutex>
namespace shivrai {
class Mempool {
public:
  void add(const Transaction&);
  std::vector<Transaction> drain(size_t max=100);
  size_t size() const;
private:
  std::vector<Transaction> txs_;
  mutable std::mutex mu_;
};
}

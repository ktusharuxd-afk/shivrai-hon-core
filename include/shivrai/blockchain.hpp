#pragma once
#include "shivrai/types.hpp"
#include "shivrai/storage.hpp"
#include <memory>
#include <mutex>
namespace shivrai {
class Blockchain {
public:
  explicit Blockchain(std::shared_ptr<Storage>);
  void load();
  Block add_block(const std::vector<Transaction>&, const std::string&, const std::string&);
  void import_block(const Block& b);  // copy as-is, preserves hash
  Block latest() const;
  uint64_t height() const;
  Block get_block(uint64_t) const;
  std::string compute_tx_hash(const Transaction&) const;
private:
  std::shared_ptr<Storage> storage_;
  Block latest_;
  mutable std::mutex mu_;
  std::string compute_block_hash(const Block&) const;
};
}

#pragma once
#include "shivrai/types.hpp"
#include "shivrai/storage.hpp"
#include <memory>
#include <mutex>
namespace shivrai {
class State {
public:
  explicit State(std::shared_ptr<Storage>);
  void apply_tx(const Transaction&);
  void apply_block(const Block&);
  uint64_t balance(const std::string&);
  void set_balance(const std::string&, uint64_t);
  void mint(const std::string&, uint64_t);
  uint64_t get_u64(const std::string&, uint64_t def=0);
  void set_u64(const std::string&, uint64_t);
  std::string get_str(const std::string&, const std::string& def="");
  void set_str(const std::string&, const std::string&);
  std::string state_root();
private:
  std::shared_ptr<Storage> storage_;
  std::mutex mu_;
};
}

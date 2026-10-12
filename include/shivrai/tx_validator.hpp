#pragma once
#include "shivrai/types.hpp"
#include "shivrai/state.hpp"
#include "shivrai/mempool.hpp"
namespace shivrai {
class TxValidator {
public:
  TxValidator(State& s, Mempool& m) : state_(s), pool_(m) {}
  bool submit(const Transaction& tx, std::string& reason);
  static constexpr uint64_t GAS_FEE = 1;
private:
  State& state_;
  Mempool& pool_;
};
}

#pragma once
#include "shivrai/state.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace shivrai {

struct ContractInfo {
  std::string address;
  std::string owner;
  std::vector<uint8_t> code;
  uint64_t deploy_block = 0;
  uint64_t call_count = 0;
};

struct ContractCallResult {
  bool success = false;
  std::string error;
  int32_t return_value = 0;
  uint64_t gas_used = 0;
  std::vector<std::string> events;
};

class ContractEngine {
public:
  explicit ContractEngine(State& s);

  std::string deploy(const std::vector<uint8_t>& wasm,
                     const std::string& owner);

  ContractCallResult call(const std::string& addr,
                          const std::string& caller,
                          const std::string& arg);

  ContractInfo get(const std::string& addr) const;
  std::vector<std::string> list() const;
  bool exists(const std::string& addr) const;

private:
  State& state_;
};

}

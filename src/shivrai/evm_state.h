// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#ifndef SHIVRAI_HON_EVM_STATE_H
#define SHIVRAI_HON_EVM_STATE_H

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace shivrai::evm {

using Address = std::array<uint8_t, 20>;
using Word = std::array<uint8_t, 32>;

struct Account {
    uint64_t nonce{0};
    uint64_t balance_hon_nano{0};
    std::vector<uint8_t> code;
    std::map<Word, Word> storage;
};

class StateDB {
public:
    Account& GetOrCreate(const Address& address);
    const Account* Get(const Address& address) const;
    bool Transfer(const Address& from, const Address& to, uint64_t amount_hon_nano, std::string& error);

private:
    std::map<Address, Account> accounts_;
};

struct ExecutionResult {
    bool success{false};
    bool reverted{false};
    uint64_t gas_used{0};
    std::vector<uint8_t> return_data;
    std::string error;
};

// Phase 5 deterministic execution kernel. This intentionally implements a
// small, consensus-testable opcode set; it is not yet a complete Ethereum VM.
ExecutionResult ExecuteBytecode(const std::vector<uint8_t>& code,
                                uint64_t gas_limit,
                                const std::vector<uint8_t>& input = {});

} // namespace shivrai::evm
#endif // SHIVRAI_HON_EVM_STATE_H

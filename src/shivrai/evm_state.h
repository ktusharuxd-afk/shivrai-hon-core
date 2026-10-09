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
    // Account access
    Account& GetOrCreate(const Address& address);
    const Account* Get(const Address& address) const;
    bool Exists(const Address& address) const;

    // Balance
    uint64_t GetBalance(const Address& address) const;
    void SetBalance(const Address& address, uint64_t amount_hon_nano);

    // Nonce
    uint64_t GetNonce(const Address& address) const;
    void SetNonce(const Address& address, uint64_t nonce);
    void IncrementNonce(const Address& address);

    // Code
    const std::vector<uint8_t>& GetCode(const Address& address) const;
    void SetCode(const Address& address, const std::vector<uint8_t>& code);

    // Storage
    Word GetStorage(const Address& address, const Word& key) const;
    void SetStorage(const Address& address, const Word& key, const Word& value);

    // Transfer (with validation)
    bool Transfer(const Address& from, const Address& to,
                  uint64_t amount_hon_nano, std::string& error);

    // Introspection
    size_t AccountCount() const { return accounts_.size(); }
    void Clear() { accounts_.clear(); }

    // Simple state hash (XOR of all account addresses + balances)
    // NOTE: placeholder; production must use Merkle Patricia Trie root.
    uint64_t StateHash() const;

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

// Deterministic execution kernel (Phase 5).
// Implements a small, consensus-testable opcode set — not a full EVM.
ExecutionResult ExecuteBytecode(const std::vector<uint8_t>& code,
                                uint64_t gas_limit,
                                const std::vector<uint8_t>& input = {});

} // namespace shivrai::evm
#endif

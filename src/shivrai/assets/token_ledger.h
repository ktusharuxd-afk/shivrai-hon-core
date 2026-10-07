#ifndef SHIVRAI_HON_TOKEN_LEDGER_H
#define SHIVRAI_HON_TOKEN_LEDGER_H
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace shivrai::assets {

class TokenLedger {
public:
    explicit TokenLedger(std::string symbol, uint8_t decimals = 18,
                         uint64_t max_supply = 0)
        : symbol_(std::move(symbol)), decimals_(decimals),
          max_supply_(max_supply), total_supply_(0) {}

    // Core
    bool mint(const std::string& to, uint64_t amount);
    bool burn(const std::string& from, uint64_t amount);
    bool transfer(const std::string& from, const std::string& to, uint64_t amount);

    // Allowance (ERC20-style approve/transferFrom)
    bool approve(const std::string& owner, const std::string& spender, uint64_t amount);
    bool transfer_from(const std::string& spender, const std::string& from,
                       const std::string& to, uint64_t amount);
    uint64_t allowance(const std::string& owner, const std::string& spender) const;

    // Freeze
    bool freeze(const std::string& addr);
    bool unfreeze(const std::string& addr);
    bool is_frozen(const std::string& addr) const;

    // View
    uint64_t balance(const std::string& who) const;
    uint64_t total_supply() const { return total_supply_; }
    uint64_t max_supply() const { return max_supply_; }
    const std::string& symbol() const { return symbol_; }
    uint8_t decimals() const { return decimals_; }

private:
    std::string symbol_;
    uint8_t decimals_;
    uint64_t max_supply_;
    uint64_t total_supply_;
    std::unordered_map<std::string, uint64_t> balances_;
    std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> allowances_;
    std::unordered_set<std::string> frozen_;
};

} // namespace shivrai::assets
#endif

#include "token_ledger.h"
#include <limits>
#include <stdexcept>

namespace shivrai::assets {

bool TokenLedger::mint(const std::string& to, uint64_t amount) {
    if (!amount) return false;
    if (is_frozen(to)) return false;
    if (max_supply_ > 0 && total_supply_ + amount > max_supply_) return false;
    auto& b = balances_[to];
    if (amount > std::numeric_limits<uint64_t>::max() - b) return false;
    b += amount;
    total_supply_ += amount;
    return true;
}

bool TokenLedger::burn(const std::string& from, uint64_t amount) {
    if (!amount) return false;
    if (is_frozen(from)) return false;
    auto it = balances_.find(from);
    if (it == balances_.end() || it->second < amount) return false;
    it->second -= amount;
    total_supply_ -= amount;
    return true;
}

bool TokenLedger::transfer(const std::string& from, const std::string& to, uint64_t amount) {
    if (!amount) return false;
    if (is_frozen(from) || is_frozen(to)) return false;
    auto it = balances_.find(from);
    if (it == balances_.end() || it->second < amount) return false;
    it->second -= amount;
    balances_[to] += amount;
    return true;
}

bool TokenLedger::approve(const std::string& owner, const std::string& spender, uint64_t amount) {
    allowances_[owner][spender] = amount;
    return true;
}

bool TokenLedger::transfer_from(const std::string& spender, const std::string& from,
                                 const std::string& to, uint64_t amount) {
    auto& allw = allowances_[from][spender];
    if (allw < amount) return false;
    if (!transfer(from, to, amount)) return false;
    allw -= amount;
    return true;
}

uint64_t TokenLedger::allowance(const std::string& owner, const std::string& spender) const {
    auto it = allowances_.find(owner);
    if (it == allowances_.end()) return 0;
    auto it2 = it->second.find(spender);
    return it2 == it->second.end() ? 0 : it2->second;
}

bool TokenLedger::freeze(const std::string& addr) {
    frozen_.insert(addr);
    return true;
}

bool TokenLedger::unfreeze(const std::string& addr) {
    frozen_.erase(addr);
    return true;
}

bool TokenLedger::is_frozen(const std::string& addr) const {
    return frozen_.count(addr) > 0;
}

uint64_t TokenLedger::balance(const std::string& who) const {
    auto it = balances_.find(who);
    return it == balances_.end() ? 0 : it->second;
}

} // namespace shivrai::assets

#include "token_ledger.h"
#include <limits>
#include <stdexcept>

namespace shivrai::assets {

bool TokenLedger::mint(const std::string& to, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!amount) return false;
    if (is_frozen(from)) return false;
    auto it = balances_.find(from);
    if (it == balances_.end() || it->second < amount) return false;
    it->second -= amount;
    total_supply_ -= amount;
    return true;
}

bool TokenLedger::transfer(const std::string& from, const std::string& to, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!amount) return false;
    if (from == to) return false;
    if (is_frozen(from) || is_frozen(to)) return false;
    auto it = balances_.find(from);
    if (it == balances_.end() || it->second < amount) return false;
    if (amount > std::numeric_limits<uint64_t>::max() - balances_[to]) return false;
    it->second -= amount;
    balances_[to] += amount;
    return true;
}

bool TokenLedger::approve(const std::string& owner, const std::string& spender, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    allowances_[owner][spender] = amount;
    return true;
}

bool TokenLedger::increase_allowance(const std::string& owner, const std::string& spender,
                                       uint64_t delta) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto& allw = allowances_[owner][spender];
    if (delta > std::numeric_limits<uint64_t>::max() - allw) return false;
    allw += delta;
    return true;
}

bool TokenLedger::decrease_allowance(const std::string& owner, const std::string& spender,
                                       uint64_t delta) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto& allw = allowances_[owner][spender];
    if (allw < delta) return false;
    allw -= delta;
    return true;
}

bool TokenLedger::transfer_from(const std::string& spender, const std::string& from,
                                 const std::string& to, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto& allw = allowances_[from][spender];
    if (allw < amount) return false;
    if (!transfer(from, to, amount)) return false;
    allw -= amount;
    return true;
}

uint64_t TokenLedger::allowance(const std::string& owner, const std::string& spender) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = allowances_.find(owner);
    if (it == allowances_.end()) return 0;
    auto it2 = it->second.find(spender);
    return it2 == it->second.end() ? 0 : it2->second;
}

bool TokenLedger::freeze(const std::string& addr) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    frozen_.insert(addr);
    return true;
}

bool TokenLedger::unfreeze(const std::string& addr) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    frozen_.erase(addr);
    return true;
}

bool TokenLedger::is_frozen(const std::string& addr) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return frozen_.count(addr) > 0;
}

uint64_t TokenLedger::balance(const std::string& who) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = balances_.find(who);
    return it == balances_.end() ? 0 : it->second;
}

uint64_t TokenLedger::total_supply() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return total_supply_;
}

size_t TokenLedger::account_count() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return balances_.size();
}

} // namespace shivrai::assets

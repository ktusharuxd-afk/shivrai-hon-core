#include "fee_market.h"
#include <vector>
namespace shivrai::fee_market {
FeeMarket::FeeMarket() = default;
void FeeMarket::SetTargetBlockGas(uint64_t g) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (g > 0) target_gas_ = g;
}
void FeeMarket::SetMaxBlockGas(uint64_t g) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (g > 0) max_gas_ = g;
}
uint64_t FeeMarket::CurrentBaseFee() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return base_fee_;
}
uint64_t FeeMarket::BaseFeeFor(uint64_t parent_gas, uint64_t parent_base) const {
    if (parent_gas == target_gas_) return parent_base;
    if (parent_gas > target_gas_) {
        uint64_t delta = parent_gas - target_gas_;
        uint64_t bump = (uint64_t)((__uint128_t)parent_base * delta / target_gas_ / 8);
        return parent_base + bump;
    }
    uint64_t delta = target_gas_ - parent_gas;
    uint64_t drop = (uint64_t)((__uint128_t)parent_base * delta / target_gas_ / 8);
    return parent_base > drop ? parent_base - drop : 0;
}
uint64_t FeeMarket::EffectiveTip(uint64_t max_fee, uint64_t priority_fee, uint64_t base_fee) const {
    if (max_fee < base_fee) return 0;
    uint64_t available = max_fee - base_fee;
    return available < priority_fee ? available : priority_fee;
}
bool FeeMarket::SubmitTx(const std::string& h, uint64_t mf, uint64_t pf, uint64_t gl) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (h.empty() || gl == 0 || gl > max_gas_) return false;
    if (txs_.count(h)) return false;
    Tx t; t.hash = h; t.max_fee = mf; t.priority_fee = pf;
    t.gas_used = gl; t.base_fee = base_fee_;
    txs_[h] = t;
    queue_.push_back(h);
    return true;
}
std::optional<Tx> FeeMarket::GetTx(const std::string& h) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = txs_.find(h);
    if (it == txs_.end()) return std::nullopt;
    return it->second;
}
size_t FeeMarket::PendingCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return queue_.size();
}
bool FeeMarket::MineBlock() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t used = 0;
    std::vector<std::string> mined;
    while (!queue_.empty() && used < max_gas_) {
        auto h = queue_.front();
        auto it = txs_.find(h);
        if (it == txs_.end()) { queue_.pop_front(); continue; }
        if (it->second.max_fee < base_fee_) { queue_.pop_front(); continue; }
        if (used + it->second.gas_used > max_gas_) break;
        used += it->second.gas_used;
        mined.push_back(h);
        queue_.pop_front();
    }
    last_block_gas_ = used;
    base_fee_ = BaseFeeFor(used, base_fee_);
    blocks_mined_++;
    return true;
}
size_t FeeMarket::BlocksMined() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return blocks_mined_;
}
uint64_t FeeMarket::LastBlockGasUsed() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return last_block_gas_;
}
}

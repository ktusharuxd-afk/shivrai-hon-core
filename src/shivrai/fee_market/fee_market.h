#ifndef SHIVRAI_HON_FEE_MARKET_H
#define SHIVRAI_HON_FEE_MARKET_H
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <string>
namespace shivrai::fee_market {
static constexpr uint64_t GWEI = 1'000'000'000ULL;
struct Tx {
    std::string hash;
    uint64_t max_fee{0};
    uint64_t priority_fee{0};
    uint64_t gas_used{0};
    uint64_t base_fee{0};
};
class FeeMarket {
public:
    FeeMarket();
    void SetTargetBlockGas(uint64_t g);
    void SetMaxBlockGas(uint64_t g);
    uint64_t CurrentBaseFee() const;
    uint64_t BaseFeeFor(uint64_t parent_gas_used, uint64_t parent_base_fee) const;
    bool SubmitTx(const std::string& hash, uint64_t max_fee, uint64_t priority_fee, uint64_t gas_limit);
    std::optional<Tx> GetTx(const std::string& hash) const;
    size_t PendingCount() const;
    uint64_t EffectiveTip(uint64_t max_fee, uint64_t priority_fee, uint64_t base_fee) const;
    bool MineBlock();
    size_t BlocksMined() const;
    uint64_t LastBlockGasUsed() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, Tx> txs_;
    std::deque<std::string> queue_;
    uint64_t target_gas_{15'000'000};
    uint64_t max_gas_{30'000'000};
    uint64_t base_fee_{GWEI};
    size_t blocks_mined_{0};
    uint64_t last_block_gas_{0};
};
}
#endif

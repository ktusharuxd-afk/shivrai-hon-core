#ifndef SHIVRAI_HON_STABLECOIN_H
#define SHIVRAI_HON_STABLECOIN_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace shivrai::stablecoin {

static constexpr uint32_t MIN_COLLATERAL_BPS       = 15000;
static constexpr uint32_t LIQUIDATION_BPS          = 13000;
static constexpr uint32_t LIQUIDATION_PENALTY_BPS  = 1000;
static constexpr uint32_t STABILITY_FEE_BPS        = 200;
static constexpr uint64_t MIN_DEBT                 = 100;
static constexpr int64_t  SECONDS_PER_YEAR         = 31536000;

using VaultId = uint64_t;

struct Vault {
    VaultId     id{0};
    std::string owner;
    std::string collateral_asset;
    uint64_t    collateral_amount{0};
    uint64_t    debt{0};
    int64_t     last_fee_accrual{0};
};

struct VaultInfo {
    VaultId     id{0};
    uint64_t    collateral{0};
    uint64_t    debt{0};
    uint64_t    collateral_value{0};
    uint32_t    collateral_ratio_bps{0};
    bool        liquidatable{false};
};

class Stablecoin {
public:
    Stablecoin();

    void SetMinCollateralBps(uint32_t bps);
    void SetLiquidationBps(uint32_t bps);
    void SetStabilityFeeBps(uint32_t bps);
    void SetMinDebt(uint64_t amount);
    uint32_t MinCollateralBps() const { return min_collateral_bps_; }
    uint32_t LiquidationBps() const   { return liquidation_bps_; }
    uint32_t StabilityFeeBps() const  { return stability_fee_bps_; }

    std::optional<VaultId> OpenVault(const std::string& owner,
                                     const std::string& collateral_asset,
                                     uint64_t collateral_amount);
    bool DepositCollateral(VaultId id, uint64_t amount);
    bool WithdrawCollateral(VaultId id, uint64_t amount, uint64_t collateral_price);
    std::optional<uint64_t> MintStable(VaultId id, uint64_t amount, uint64_t collateral_price);
    bool RepayStable(VaultId id, uint64_t amount, uint64_t collateral_price);
    bool CloseVault(VaultId id, uint64_t collateral_price);
    bool Liquidate(VaultId id, uint64_t collateral_price);

    bool StabilityPoolDeposit(const std::string& depositor, uint64_t amount);
    bool StabilityPoolWithdraw(const std::string& depositor, uint64_t amount);
    uint64_t StabilityPoolBalance() const;
    uint64_t StabilityPoolBalanceOf(const std::string& depositor) const;

    void PSMDeposit(uint64_t usd_amount);
    std::optional<uint64_t> PSMSwapIn(uint64_t usd_amount);
    std::optional<uint64_t> PSMSwapOut(uint64_t shon_amount);
    uint64_t PSMReserve() const;

    std::optional<Vault>     GetVault(VaultId id) const;
    std::optional<VaultInfo> GetVaultInfo(VaultId id, uint64_t collateral_price) const;
    std::vector<VaultId>     VaultsOf(const std::string& owner) const;
    uint64_t TotalSupply() const;
    uint64_t TotalCollateral(const std::string& asset) const;
    size_t   VaultCount() const;

    void EmergencyShutdown();
    void Resume();
    bool IsShutdown() const;

private:
    void     AccrueFee(Vault& v, int64_t now);
    uint32_t ComputeRatio(uint64_t collateral_value, uint64_t debt) const;

    mutable std::recursive_mutex cs_;
    std::map<VaultId, Vault>                  vaults_;
    std::map<std::string, std::set<VaultId>>  owner_vaults_;
    std::map<std::string, uint64_t>           stability_pool_;
    uint64_t stability_pool_total_{0};
    uint64_t psm_reserve_{0};
    uint64_t total_supply_{0};
    VaultId  next_vault_id_{1};

    uint32_t min_collateral_bps_{MIN_COLLATERAL_BPS};
    uint32_t liquidation_bps_{LIQUIDATION_BPS};
    uint32_t stability_fee_bps_{STABILITY_FEE_BPS};
    uint64_t min_debt_{MIN_DEBT};
    bool     shutdown_{false};
};

}

#endif

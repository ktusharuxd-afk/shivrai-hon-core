#include "stablecoin.h"
#include <algorithm>
#include <ctime>

namespace shivrai::stablecoin {

Stablecoin::Stablecoin() = default;

void Stablecoin::SetMinCollateralBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    min_collateral_bps_ = bps;
}
void Stablecoin::SetLiquidationBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    liquidation_bps_ = bps;
}
void Stablecoin::SetStabilityFeeBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    stability_fee_bps_ = bps;
}
void Stablecoin::SetMinDebt(uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    min_debt_ = amount;
}

uint32_t Stablecoin::ComputeRatio(uint64_t collateral_value, uint64_t debt) const {
    if (debt == 0) return UINT32_MAX;
    __uint128_t num = (__uint128_t)collateral_value * 10000ULL;
    uint64_t r = (uint64_t)(num / debt);
    return r > UINT32_MAX ? UINT32_MAX : (uint32_t)r;
}

void Stablecoin::AccrueFee(Vault& v, int64_t now) {
    if (v.last_fee_accrual == 0 || v.debt == 0) {
        v.last_fee_accrual = now;
        return;
    }
    int64_t elapsed = now - v.last_fee_accrual;
    if (elapsed <= 0) return;
    __uint128_t num = (__uint128_t)v.debt * stability_fee_bps_ * (uint64_t)elapsed;
    __uint128_t den = (__uint128_t)10000ULL * (uint64_t)SECONDS_PER_YEAR;
    uint64_t fee = (uint64_t)(num / den);
    v.debt += fee;
    total_supply_ += fee;
    v.last_fee_accrual = now;
}

std::optional<VaultId> Stablecoin::OpenVault(const std::string& owner,
                                             const std::string& collateral_asset,
                                             uint64_t collateral_amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (shutdown_) return std::nullopt;
    if (collateral_amount == 0) return std::nullopt;
    VaultId id = next_vault_id_++;
    Vault v;
    v.id = id;
    v.owner = owner;
    v.collateral_asset = collateral_asset;
    v.collateral_amount = collateral_amount;
    v.debt = 0;
    v.last_fee_accrual = (int64_t)std::time(nullptr);
    vaults_[id] = v;
    owner_vaults_[owner].insert(id);
    return id;
}

bool Stablecoin::DepositCollateral(VaultId id, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end() || amount == 0) return false;
    it->second.collateral_amount += amount;
    return true;
}

bool Stablecoin::WithdrawCollateral(VaultId id, uint64_t amount, uint64_t collateral_price) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end()) return false;
    if (amount == 0 || amount > it->second.collateral_amount) return false;
    AccrueFee(it->second, (int64_t)std::time(nullptr));
    uint64_t new_coll = it->second.collateral_amount - amount;
    if (it->second.debt > 0) {
        __uint128_t cv = (__uint128_t)new_coll * collateral_price;
        uint64_t cv64 = cv > UINT64_MAX ? UINT64_MAX : (uint64_t)cv;
        if (ComputeRatio(cv64, it->second.debt) < min_collateral_bps_) return false;
    }
    it->second.collateral_amount = new_coll;
    return true;
}

std::optional<uint64_t> Stablecoin::MintStable(VaultId id, uint64_t amount, uint64_t collateral_price) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (shutdown_) return std::nullopt;
    auto it = vaults_.find(id);
    if (it == vaults_.end() || amount == 0) return std::nullopt;
    AccrueFee(it->second, (int64_t)std::time(nullptr));
    uint64_t new_debt = it->second.debt + amount;
    if (new_debt < min_debt_) return std::nullopt;
    __uint128_t cv = (__uint128_t)it->second.collateral_amount * collateral_price;
    uint64_t cv64 = cv > UINT64_MAX ? UINT64_MAX : (uint64_t)cv;
    if (ComputeRatio(cv64, new_debt) < min_collateral_bps_) return std::nullopt;
    it->second.debt = new_debt;
    total_supply_ += amount;
    return amount;
}

bool Stablecoin::RepayStable(VaultId id, uint64_t amount, uint64_t /*collateral_price*/) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end() || amount == 0) return false;
    AccrueFee(it->second, (int64_t)std::time(nullptr));
    uint64_t actual = std::min(amount, it->second.debt);
    it->second.debt -= actual;
    total_supply_ -= actual;
    return true;
}

bool Stablecoin::CloseVault(VaultId id, uint64_t /*collateral_price*/) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end()) return false;
    AccrueFee(it->second, (int64_t)std::time(nullptr));
    if (it->second.debt != 0) return false;
    owner_vaults_[it->second.owner].erase(id);
    vaults_.erase(it);
    return true;
}

bool Stablecoin::Liquidate(VaultId id, uint64_t collateral_price) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end()) return false;
    AccrueFee(it->second, (int64_t)std::time(nullptr));
    if (it->second.debt == 0) return false;
    __uint128_t cv = (__uint128_t)it->second.collateral_amount * collateral_price;
    uint64_t cv64 = cv > UINT64_MAX ? UINT64_MAX : (uint64_t)cv;
    if (ComputeRatio(cv64, it->second.debt) >= liquidation_bps_) return false;

    uint64_t debt = it->second.debt;
    if (stability_pool_total_ >= debt && stability_pool_total_ > 0) {
        for (auto& [dep, bal] : stability_pool_) {
            __uint128_t share = (__uint128_t)bal * debt / stability_pool_total_;
            bal -= (uint64_t)share;
        }
        stability_pool_total_ -= debt;
        for (auto i2 = stability_pool_.begin(); i2 != stability_pool_.end(); ) {
            if (i2->second == 0) i2 = stability_pool_.erase(i2);
            else ++i2;
        }
    } else {
        stability_pool_.clear();
        stability_pool_total_ = 0;
    }
    total_supply_ -= debt;
    owner_vaults_[it->second.owner].erase(id);
    vaults_.erase(it);
    return true;
}

bool Stablecoin::StabilityPoolDeposit(const std::string& depositor, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (amount == 0) return false;
    stability_pool_[depositor] += amount;
    stability_pool_total_ += amount;
    return true;
}

bool Stablecoin::StabilityPoolWithdraw(const std::string& depositor, uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = stability_pool_.find(depositor);
    if (it == stability_pool_.end()) return false;
    if (amount == 0 || amount > it->second) return false;
    it->second -= amount;
    stability_pool_total_ -= amount;
    if (it->second == 0) stability_pool_.erase(it);
    return true;
}

uint64_t Stablecoin::StabilityPoolBalance() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return stability_pool_total_;
}

uint64_t Stablecoin::StabilityPoolBalanceOf(const std::string& depositor) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = stability_pool_.find(depositor);
    return it == stability_pool_.end() ? 0 : it->second;
}

void Stablecoin::PSMDeposit(uint64_t usd_amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    psm_reserve_ += usd_amount;
}

std::optional<uint64_t> Stablecoin::PSMSwapIn(uint64_t usd_amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (usd_amount == 0 || psm_reserve_ < usd_amount) return std::nullopt;
    psm_reserve_ -= usd_amount;
    total_supply_ += usd_amount;
    return usd_amount;
}

std::optional<uint64_t> Stablecoin::PSMSwapOut(uint64_t shon_amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (shon_amount == 0 || psm_reserve_ < shon_amount) return std::nullopt;
    if (total_supply_ < shon_amount) return std::nullopt;
    psm_reserve_ -= shon_amount;
    total_supply_ -= shon_amount;
    return shon_amount;
}

uint64_t Stablecoin::PSMReserve() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return psm_reserve_;
}

std::optional<Vault> Stablecoin::GetVault(VaultId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end()) return std::nullopt;
    return it->second;
}

std::optional<VaultInfo> Stablecoin::GetVaultInfo(VaultId id, uint64_t collateral_price) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = vaults_.find(id);
    if (it == vaults_.end()) return std::nullopt;
    VaultInfo vi;
    vi.id = it->second.id;
    vi.collateral = it->second.collateral_amount;
    vi.debt = it->second.debt;
    __uint128_t cv = (__uint128_t)vi.collateral * collateral_price;
    vi.collateral_value = cv > UINT64_MAX ? UINT64_MAX : (uint64_t)cv;
    vi.collateral_ratio_bps = ComputeRatio(vi.collateral_value, vi.debt);
    vi.liquidatable = (vi.debt > 0) && (vi.collateral_ratio_bps < liquidation_bps_);
    return vi;
}

std::vector<VaultId> Stablecoin::VaultsOf(const std::string& owner) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<VaultId> out;
    auto it = owner_vaults_.find(owner);
    if (it != owner_vaults_.end()) out.assign(it->second.begin(), it->second.end());
    return out;
}

uint64_t Stablecoin::TotalSupply() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return total_supply_;
}

uint64_t Stablecoin::TotalCollateral(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    uint64_t total = 0;
    for (auto& [id, v] : vaults_) {
        if (v.collateral_asset == asset) total += v.collateral_amount;
    }
    return total;
}

size_t Stablecoin::VaultCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return vaults_.size();
}

void Stablecoin::EmergencyShutdown() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    shutdown_ = true;
}

void Stablecoin::Resume() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    shutdown_ = false;
}
bool Stablecoin::IsShutdown() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return shutdown_;
}

}

#include "shivrai/treasury/treasury.h"

#include <algorithm>
#include <limits>

namespace shivrai::treasury {

bool FeeAllocation::valid() const
{
    const unsigned total = static_cast<unsigned>(security_percent) + burn_percent + ecosystem_percent + developer_percent + reserve_percent;
    return total == 100;
}

AllocationResult AllocateFee(uint64_t fee, const FeeAllocation& a)
{
    if (!a.valid()) return {};
    AllocationResult r;
    const auto part = [fee](uint8_t pct) -> uint64_t {
        return (fee / 100U) * pct + ((fee % 100U) * pct) / 100U;
    };
    r.security = part(a.security_percent);
    r.burn = part(a.burn_percent);
    r.ecosystem = part(a.ecosystem_percent);
    r.developer = part(a.developer_percent);
    r.reserve = part(a.reserve_percent);
    r.accounted = r.security + r.burn + r.ecosystem + r.developer + r.reserve;
    // Deterministically assign any integer-division remainder to security.
    if (r.accounted < fee) r.security += fee - r.accounted;
    r.accounted = r.security + r.burn + r.ecosystem + r.developer + r.reserve;
    return r;
}

uint64_t TreasuryBalance::total_controlled() const
{
    return security + ecosystem + developer + reserve;
}

TreasuryLedger::TreasuryLedger(FeeAllocation allocation) : m_allocation(allocation) {}

bool TreasuryLedger::RecordNetworkFee(uint64_t fee)
{
    if (!m_allocation.valid()) return false;
    const auto r = AllocateFee(fee, m_allocation);
    if (r.accounted != fee) return false;
    if (std::numeric_limits<uint64_t>::max() - m_balance.security < r.security) return false;
    if (std::numeric_limits<uint64_t>::max() - m_balance.burned < r.burn) return false;
    if (std::numeric_limits<uint64_t>::max() - m_balance.ecosystem < r.ecosystem) return false;
    if (std::numeric_limits<uint64_t>::max() - m_balance.developer < r.developer) return false;
    if (std::numeric_limits<uint64_t>::max() - m_balance.reserve < r.reserve) return false;
    m_balance.security += r.security;
    m_balance.burned += r.burn;
    m_balance.ecosystem += r.ecosystem;
    m_balance.developer += r.developer;
    m_balance.reserve += r.reserve;
    return true;
}

bool Timelock::ready(uint64_t current_height) const
{
    return current_height >= queued_at_height && (current_height - queued_at_height) >= delay_blocks;
}

MultisigTreasury::MultisigTreasury(std::vector<std::string> signers, uint8_t threshold,
                                   uint64_t timelock_blocks)
    : m_signers(std::move(signers)), m_threshold(threshold), m_timelock_blocks(timelock_blocks) {}

bool MultisigTreasury::valid() const
{
    if (m_signers.empty() || m_threshold == 0 || m_threshold > m_signers.size()) return false;
    std::unordered_set<std::string> unique;
    for (const auto& signer : m_signers) {
        if (signer.empty() || !unique.insert(signer).second) return false;
    }
    return true;
}

bool MultisigTreasury::approve(TreasuryProposal& proposal, const std::string& signer) const
{
    if (!valid() || proposal.executed) return false;
    if (std::find(m_signers.begin(), m_signers.end(), signer) == m_signers.end()) return false;
    proposal.approvals.insert(signer);
    return true;
}

bool MultisigTreasury::executable(const TreasuryProposal& proposal, uint64_t current_height) const
{
    return valid() && !proposal.executed && proposal.amount > 0 &&
           proposal.approvals.size() >= m_threshold &&
           proposal.timelock.ready(current_height);
}

size_t MultisigTreasury::approval_count(const TreasuryProposal& proposal) const
{
    return proposal.approvals.size();
}

} // namespace shivrai::treasury

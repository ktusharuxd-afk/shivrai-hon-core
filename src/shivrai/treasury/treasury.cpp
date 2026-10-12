#include "shivrai/treasury/treasury.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace shivrai::treasury {

bool FeeAllocation::valid() const
{
    const unsigned total = static_cast<unsigned>(security_percent) +
                           burn_percent + ecosystem_percent +
                           developer_percent + reserve_percent;
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
    if (r.accounted < fee) r.security += fee - r.accounted;
    r.accounted = r.security + r.burn + r.ecosystem + r.developer + r.reserve;
    return r;
}

uint64_t TreasuryBalance::total_controlled() const
{
    return security + ecosystem + developer + reserve;
}

uint64_t TreasuryBalance::total_all() const
{
    return total_controlled() + burned;
}

TreasuryLedger::TreasuryLedger(FeeAllocation allocation) : m_allocation(allocation) {}

bool TreasuryLedger::RecordNetworkFee(uint64_t fee)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
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

bool TreasuryLedger::deposit(const std::string& category, uint64_t amount)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!amount) return false;

    uint64_t* target = nullptr;
    if (category == "security")       target = &m_balance.security;
    else if (category == "ecosystem") target = &m_balance.ecosystem;
    else if (category == "developer") target = &m_balance.developer;
    else if (category == "reserve")   target = &m_balance.reserve;
    else return false;

    if (std::numeric_limits<uint64_t>::max() - *target < amount) return false;
    *target += amount;
    return true;
}

bool TreasuryLedger::withdraw(const std::string& category, uint64_t amount)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!amount) return false;

    uint64_t* target = nullptr;
    if (category == "security")       target = &m_balance.security;
    else if (category == "ecosystem") target = &m_balance.ecosystem;
    else if (category == "developer") target = &m_balance.developer;
    else if (category == "reserve")   target = &m_balance.reserve;
    else return false;

    if (*target < amount) return false;
    *target -= amount;
    return true;
}

const TreasuryBalance& TreasuryLedger::balance() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_balance;
}

const FeeAllocation& TreasuryLedger::allocation() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_allocation;
}

uint64_t TreasuryLedger::total_controlled() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_balance.total_controlled();
}

uint64_t TreasuryLedger::total_burned() const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_balance.burned;
}

void TreasuryLedger::reset()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_balance = TreasuryBalance{};
}

bool Timelock::ready(uint64_t current_height) const
{
    return current_height >= queued_at_height &&
           (current_height - queued_at_height) >= delay_blocks;
}

MultisigTreasury::MultisigTreasury(std::vector<std::string> signers, uint8_t threshold,
                                   uint64_t timelock_blocks)
    : m_signers(std::move(signers)), m_threshold(threshold),
      m_timelock_blocks(timelock_blocks) {}

bool MultisigTreasury::valid() const
{
    if (m_signers.empty() || m_threshold == 0 ||
        m_threshold > m_signers.size()) return false;
    std::unordered_set<std::string> unique;
    for (const auto& signer : m_signers) {
        if (signer.empty() || !unique.insert(signer).second) return false;
    }
    return true;
}

bool MultisigTreasury::approve(TreasuryProposal& proposal, const std::string& signer) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!valid() || proposal.executed || proposal.cancelled) return false;
    if (std::find(m_signers.begin(), m_signers.end(), signer) == m_signers.end())
        return false;
    proposal.approvals.insert(signer);
    return true;
}

bool MultisigTreasury::executable(const TreasuryProposal& proposal,
                                   uint64_t current_height) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return valid() && !proposal.executed && !proposal.cancelled &&
           proposal.amount > 0 &&
           proposal.approvals.size() >= m_threshold &&
           proposal.timelock.ready(current_height);
}

bool MultisigTreasury::execute(TreasuryProposal& proposal,
                                uint64_t current_height) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!executable(proposal, current_height)) return false;
    proposal.executed = true;
    return true;
}

bool MultisigTreasury::cancel(TreasuryProposal& proposal,
                               const std::string& signer) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!valid() || proposal.executed || proposal.cancelled) return false;
    if (std::find(m_signers.begin(), m_signers.end(), signer) == m_signers.end())
        return false;
    proposal.cancelled = true;
    return true;
}

size_t MultisigTreasury::approval_count(const TreasuryProposal& proposal) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return proposal.approvals.size();
}

TreasuryProposal MultisigTreasury::create_proposal(const std::string& id,
                                                    const std::string& dest,
                                                    uint64_t amount,
                                                    uint64_t current_height) const
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    TreasuryProposal p;
    p.id = id;
    p.destination = dest;
    p.amount = amount;
    p.created_height = current_height;
    p.timelock.delay_blocks = m_timelock_blocks;
    p.timelock.queued_at_height = current_height;
    return p;
}

} // namespace shivrai::treasury

#ifndef SHIVRAI_HON_TREASURY_H
#define SHIVRAI_HON_TREASURY_H

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>

namespace shivrai::treasury {

struct FeeAllocation {
    uint8_t security_percent{50};
    uint8_t burn_percent{20};
    uint8_t ecosystem_percent{15};
    uint8_t developer_percent{10};
    uint8_t reserve_percent{5};

    bool valid() const;
};

struct AllocationResult {
    uint64_t security{0};
    uint64_t burn{0};
    uint64_t ecosystem{0};
    uint64_t developer{0};
    uint64_t reserve{0};
    uint64_t accounted{0};
};

AllocationResult AllocateFee(uint64_t fee, const FeeAllocation& allocation);

struct TreasuryBalance {
    uint64_t security{0};
    uint64_t ecosystem{0};
    uint64_t developer{0};
    uint64_t reserve{0};
    uint64_t burned{0};

    uint64_t total_controlled() const;
};

class TreasuryLedger {
public:
    explicit TreasuryLedger(FeeAllocation allocation = {});

    bool RecordNetworkFee(uint64_t fee);
    const TreasuryBalance& balance() const { return m_balance; }
    const FeeAllocation& allocation() const { return m_allocation; }

private:
    FeeAllocation m_allocation;
    TreasuryBalance m_balance;
};

struct Timelock {
    uint64_t delay_blocks{144};
    uint64_t queued_at_height{0};

    bool ready(uint64_t current_height) const;
};

struct TreasuryProposal {
    std::string id;
    std::string destination;
    uint64_t amount{0};
    uint64_t created_height{0};
    Timelock timelock{};
    bool executed{false};
    std::unordered_set<std::string> approvals;
};

class MultisigTreasury {
public:
    MultisigTreasury(std::vector<std::string> signers, uint8_t threshold,
                     uint64_t timelock_blocks = 144);

    bool valid() const;
    bool approve(TreasuryProposal& proposal, const std::string& signer) const;
    bool executable(const TreasuryProposal& proposal, uint64_t current_height) const;
    size_t approval_count(const TreasuryProposal& proposal) const;
    uint8_t threshold() const { return m_threshold; }
    uint64_t timelock_blocks() const { return m_timelock_blocks; }
    const std::vector<std::string>& signers() const { return m_signers; }

private:
    std::vector<std::string> m_signers;
    uint8_t m_threshold{0};
    uint64_t m_timelock_blocks{144};
};

} // namespace shivrai::treasury

#endif

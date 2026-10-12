#ifndef SHIVRAI_HON_TREASURY_H
#define SHIVRAI_HON_TREASURY_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

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
    uint64_t total_all() const;
};

class TreasuryLedger {
public:
    explicit TreasuryLedger(FeeAllocation allocation = {});

    // Fee recording
    bool RecordNetworkFee(uint64_t fee);

    // Manual deposit/withdraw (owner-controlled)
    bool deposit(const std::string& category, uint64_t amount);
    bool withdraw(const std::string& category, uint64_t amount);

    // View
    const TreasuryBalance& balance() const;
    const FeeAllocation& allocation() const;
    uint64_t total_controlled() const;
    uint64_t total_burned() const;

    // Reset (for testing / migrations)
    void reset();

private:
    FeeAllocation m_allocation;
    TreasuryBalance m_balance;
    mutable std::recursive_mutex m_mutex;
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
    bool cancelled{false};
    std::unordered_set<std::string> approvals;
};

class MultisigTreasury {
public:
    MultisigTreasury(std::vector<std::string> signers, uint8_t threshold,
                     uint64_t timelock_blocks = 144);

    bool valid() const;
    bool approve(TreasuryProposal& proposal, const std::string& signer) const;
    bool executable(const TreasuryProposal& proposal, uint64_t current_height) const;
    bool execute(TreasuryProposal& proposal, uint64_t current_height) const;
    bool cancel(TreasuryProposal& proposal, const std::string& signer) const;

    size_t approval_count(const TreasuryProposal& proposal) const;
    uint8_t threshold() const { return m_threshold; }
    uint64_t timelock_blocks() const { return m_timelock_blocks; }
    const std::vector<std::string>& signers() const { return m_signers; }

    // Create a new proposal (any signer can)
    TreasuryProposal create_proposal(const std::string& id,
                                      const std::string& dest,
                                      uint64_t amount,
                                      uint64_t current_height) const;

private:
    std::vector<std::string> m_signers;
    uint8_t m_threshold{0};
    uint64_t m_timelock_blocks{144};
    mutable std::recursive_mutex m_mutex;
};

} // namespace shivrai::treasury

#endif

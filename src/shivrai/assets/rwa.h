#ifndef SHIVRAI_HON_RWA_H
#define SHIVRAI_HON_RWA_H
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <mutex>

namespace shivrai::assets {

enum class RWAType { REAL_ESTATE, INVOICE, COMMODITY, FUND, CERTIFICATE, OTHER };
enum class RWAStatus { PENDING, ACTIVE, FROZEN, REDEEMED };

struct RWAAsset {
    std::string id;
    std::string owner;
    RWAType type;
    std::string description;
    uint64_t value{0};
    std::string currency;
    std::string issuer;
    uint64_t issued_at{0};
    uint64_t expires_at{0};
    RWAStatus status{RWAStatus::PENDING};
    std::string legal_doc_hash;
    std::string jurisdiction;
};

struct RWATransfer {
    std::string asset_id;
    std::string from;
    std::string to;
    uint64_t timestamp{0};
    std::string tx_ref;
};

class RWARegistry {
public:
    std::string register_asset(const std::string& owner, RWAType type,
                                const std::string& description, uint64_t value,
                                const std::string& currency, const std::string& issuer,
                                const std::string& jurisdiction,
                                const std::string& legal_doc_hash = "");

    bool activate(const std::string& asset_id);
    bool freeze(const std::string& asset_id);
    bool redeem(const std::string& asset_id);
    bool transfer(const std::string& asset_id, const std::string& to, const std::string& tx_ref);

    const RWAAsset* get(const std::string& id) const;
    std::vector<RWAAsset> by_owner(const std::string& owner) const;
    std::vector<RWATransfer> transfer_history(const std::string& asset_id) const;

    size_t count() const { return assets_.size(); }

private:
    mutable std::recursive_mutex mutex_;
    std::unordered_map<std::string, RWAAsset> assets_;
    std::unordered_map<std::string, std::vector<RWATransfer>> transfers_;
    std::string gen_id() const;
};

} // namespace shivrai::assets
#endif

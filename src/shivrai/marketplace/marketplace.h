#ifndef SHIVRAI_HON_MARKETPLACE_H
#define SHIVRAI_HON_MARKETPLACE_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace shivrai::marketplace {

using ListingId = uint64_t;
using NFTId     = uint64_t;
using BidId     = uint64_t;

static constexpr uint32_t PLATFORM_FEE_BPS = 250;
static constexpr uint32_t BPS = 10000;

enum class ListingKind { FixedPrice, Auction };

struct Listing {
    ListingId   id{0};
    std::string seller;
    NFTId       nft_id{0};
    ListingKind kind{ListingKind::FixedPrice};
    uint64_t    price{0};
    uint64_t    highest_bid{0};
    BidId       highest_bid_id{0};
    std::string highest_bidder;
    int64_t     created_at{0};
    int64_t     expires_at{0};
    bool        active{true};
};

struct Bid {
    BidId       id{0};
    ListingId   listing_id{0};
    std::string bidder;
    uint64_t    amount{0};
    int64_t     created_at{0};
};

struct SaleSplit {
    uint64_t    seller_proceeds{0};
    uint64_t    royalty_paid{0};
    uint64_t    platform_fee{0};
    std::string royalty_recipient;
};

class Marketplace {
public:
    Marketplace();

    std::optional<ListingId> ListFixedPrice(const std::string& seller,
                                            NFTId nft_id,
                                            uint64_t price);
    std::optional<ListingId> ListAuction(const std::string& seller,
                                         NFTId nft_id,
                                         uint64_t reserve_price,
                                         int64_t duration_sec);
    bool CancelListing(const std::string& seller, ListingId id);

    std::optional<BidId> PlaceBid(const std::string& bidder,
                                  ListingId id,
                                  uint64_t amount);

    std::optional<SaleSplit> BuyNow(const std::string& buyer,
                                    ListingId id,
                                    uint32_t royalty_bps,
                                    const std::string& royalty_recipient);
    std::optional<SaleSplit> SettleAuction(const std::string& caller,
                                           ListingId id,
                                           uint32_t royalty_bps,
                                           const std::string& royalty_recipient);

    bool ExpireAuction(ListingId id);

    std::optional<Listing> GetListing(ListingId id) const;
    std::optional<Bid>     GetBid(BidId id) const;
    std::vector<ListingId> ActiveListings() const;
    std::vector<ListingId> ListingsBySeller(const std::string& seller) const;
    std::vector<BidId>     BidsForListing(ListingId id) const;
    size_t                 ListingCount() const;
    size_t                 BidCount() const;

    void SetPlatformFeeBps(uint32_t bps);
    uint32_t PlatformFeeBps() const;

private:
    std::optional<SaleSplit> Settle(Listing& l, const std::string& buyer, uint64_t amount,
                                    uint32_t royalty_bps, const std::string& royalty_recipient);

    mutable std::recursive_mutex cs_;
    std::map<ListingId, Listing> listings_;
    std::map<BidId, Bid>         bids_;
    std::map<ListingId, std::vector<BidId>> bids_by_listing_;
    std::map<std::string, std::vector<ListingId>> listings_by_seller_;

    ListingId next_listing_id_{1};
    BidId     next_bid_id_{1};
    uint32_t  platform_fee_bps_{PLATFORM_FEE_BPS};
};

}
#endif
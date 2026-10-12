#include "marketplace.h"
#include <algorithm>
#include <ctime>

namespace shivrai::marketplace {

Marketplace::Marketplace() = default;

void Marketplace::SetPlatformFeeBps(uint32_t bps) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (bps <= BPS) platform_fee_bps_ = bps;
}

uint32_t Marketplace::PlatformFeeBps() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return platform_fee_bps_;
}

std::optional<ListingId> Marketplace::ListFixedPrice(const std::string& seller,
                                                     NFTId nft_id,
                                                     uint64_t price) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (seller.empty() || nft_id == 0 || price == 0) return std::nullopt;
    ListingId id = next_listing_id_++;
    Listing l;
    l.id = id;
    l.seller = seller;
    l.nft_id = nft_id;
    l.kind = ListingKind::FixedPrice;
    l.price = price;
    l.created_at = (int64_t)std::time(nullptr);
    l.expires_at = 0;
    listings_[id] = l;
    listings_by_seller_[seller].push_back(id);
    return id;
}

std::optional<ListingId> Marketplace::ListAuction(const std::string& seller,
                                                  NFTId nft_id,
                                                  uint64_t reserve_price,
                                                  int64_t duration_sec) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (seller.empty() || nft_id == 0 || reserve_price == 0 || duration_sec <= 0)
        return std::nullopt;
    ListingId id = next_listing_id_++;
    Listing l;
    l.id = id;
    l.seller = seller;
    l.nft_id = nft_id;
    l.kind = ListingKind::Auction;
    l.price = reserve_price;
    l.created_at = (int64_t)std::time(nullptr);
    l.expires_at = l.created_at + duration_sec;
    listings_[id] = l;
    listings_by_seller_[seller].push_back(id);
    return id;
}

bool Marketplace::CancelListing(const std::string& seller, ListingId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_.find(id);
    if (it == listings_.end()) return false;
    if (!it->second.active) return false;
    if (it->second.seller != seller) return false;
    if (it->second.kind == ListingKind::Auction && it->second.highest_bid > 0)
        return false;  // cannot cancel auction with bids
    it->second.active = false;
    return true;
}

std::optional<BidId> Marketplace::PlaceBid(const std::string& bidder,
                                           ListingId id,
                                           uint64_t amount) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_.find(id);
    if (it == listings_.end()) return std::nullopt;
    if (!it->second.active) return std::nullopt;
    if (it->second.kind != ListingKind::Auction) return std::nullopt;
    if (bidder.empty() || bidder == it->second.seller) return std::nullopt;
    if (amount < it->second.price) return std::nullopt;
    if (amount <= it->second.highest_bid) return std::nullopt;

    int64_t now = (int64_t)std::time(nullptr);
    if (it->second.expires_at > 0 && now >= it->second.expires_at) return std::nullopt;

    BidId bid_id = next_bid_id_++;
    Bid b;
    b.id = bid_id;
    b.listing_id = id;
    b.bidder = bidder;
    b.amount = amount;
    b.created_at = now;
    bids_[bid_id] = b;
    bids_by_listing_[id].push_back(bid_id);

    it->second.highest_bid = amount;
    it->second.highest_bid_id = bid_id;
    it->second.highest_bidder = bidder;
    return bid_id;
}

std::optional<SaleSplit> Marketplace::Settle(Listing& l,
                                             const std::string& buyer,
                                             uint64_t amount,
                                             uint32_t royalty_bps,
                                             const std::string& royalty_recipient) {
    SaleSplit s;
    s.royalty_paid = (uint64_t)((__uint128_t)amount * royalty_bps / BPS);
    s.platform_fee = (uint64_t)((__uint128_t)amount * platform_fee_bps_ / BPS);
    if (s.royalty_paid + s.platform_fee > amount) {
        s.royalty_paid = 0;
        s.platform_fee = (uint64_t)((__uint128_t)amount * platform_fee_bps_ / BPS);
    }
    s.seller_proceeds = amount - s.royalty_paid - s.platform_fee;
    s.royalty_recipient = royalty_recipient;

    l.active = false;
    l.highest_bidder = buyer;
    return s;
}

std::optional<SaleSplit> Marketplace::BuyNow(const std::string& buyer,
                                             ListingId id,
                                             uint32_t royalty_bps,
                                             const std::string& royalty_recipient) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_.find(id);
    if (it == listings_.end()) return std::nullopt;
    if (!it->second.active) return std::nullopt;
    if (it->second.kind != ListingKind::FixedPrice) return std::nullopt;
    if (buyer.empty() || buyer == it->second.seller) return std::nullopt;
    return Settle(it->second, buyer, it->second.price, royalty_bps, royalty_recipient);
}

std::optional<SaleSplit> Marketplace::SettleAuction(const std::string& caller,
                                                    ListingId id,
                                                    uint32_t royalty_bps,
                                                    const std::string& royalty_recipient) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_.find(id);
    if (it == listings_.end()) return std::nullopt;
    if (!it->second.active) return std::nullopt;
    if (it->second.kind != ListingKind::Auction) return std::nullopt;
    if (it->second.highest_bid == 0) return std::nullopt;

    int64_t now = (int64_t)std::time(nullptr);
    bool ended = (it->second.expires_at > 0 && now >= it->second.expires_at);
    bool caller_is_seller = (caller == it->second.seller);
    if (!ended && !caller_is_seller) return std::nullopt;

    return Settle(it->second,
                  it->second.highest_bidder,
                  it->second.highest_bid,
                  royalty_bps,
                  royalty_recipient);
}

bool Marketplace::ExpireAuction(ListingId id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_.find(id);
    if (it == listings_.end()) return false;
    if (!it->second.active) return false;
    if (it->second.kind != ListingKind::Auction) return false;
    if (it->second.highest_bid > 0) return false;
    int64_t now = (int64_t)std::time(nullptr);
    if (it->second.expires_at > 0 && now < it->second.expires_at) return false;
    it->second.active = false;
    return true;
}

std::optional<Listing> Marketplace::GetListing(ListingId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_.find(id);
    if (it == listings_.end()) return std::nullopt;
    return it->second;
}

std::optional<Bid> Marketplace::GetBid(BidId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = bids_.find(id);
    if (it == bids_.end()) return std::nullopt;
    return it->second;
}

std::vector<ListingId> Marketplace::ActiveListings() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<ListingId> out;
    for (auto& [id, l] : listings_) if (l.active) out.push_back(id);
    return out;
}

std::vector<ListingId> Marketplace::ListingsBySeller(const std::string& seller) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = listings_by_seller_.find(seller);
    return it == listings_by_seller_.end() ? std::vector<ListingId>{} : it->second;
}

std::vector<BidId> Marketplace::BidsForListing(ListingId id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = bids_by_listing_.find(id);
    return it == bids_by_listing_.end() ? std::vector<BidId>{} : it->second;
}

size_t Marketplace::ListingCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return listings_.size();
}

size_t Marketplace::BidCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return bids_.size();
}

}
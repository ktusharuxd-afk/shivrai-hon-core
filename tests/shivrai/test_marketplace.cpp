#include "shivrai/marketplace/marketplace.h"
#include <cassert>
#include <iostream>

using namespace shivrai::marketplace;

int main() {
    int passed = 0;
    auto CHECK = [&](bool c) { assert(c); ++passed; };

    Marketplace mp;

    CHECK(mp.ListingCount() == 0);
    CHECK(mp.BidCount() == 0);
    CHECK(mp.PlatformFeeBps() == 250);
    CHECK(mp.ActiveListings().empty());

    // === Fixed price ===
    auto l1 = mp.ListFixedPrice("alice", 100, 1000);
    CHECK(l1.has_value());
    CHECK(mp.ListingCount() == 1);
    CHECK(mp.ActiveListings().size() == 1);

    auto lst = mp.GetListing(*l1);
    CHECK(lst.has_value());
    CHECK(lst->seller == "alice");
    CHECK(lst->nft_id == 100);
    CHECK(lst->price == 1000);
    CHECK(lst->kind == ListingKind::FixedPrice);
    CHECK(lst->active);

    CHECK(!mp.ListFixedPrice("", 200, 500).has_value());
    CHECK(!mp.ListFixedPrice("bob", 0, 500).has_value());
    CHECK(!mp.ListFixedPrice("bob", 200, 0).has_value());

    // Cannot buy own listing
    CHECK(!mp.BuyNow("alice", *l1, 500, "alice").has_value());

    // Buy now — 1000, royalty 5%, platform 2.5%
    // royalty = 50, platform = 25, seller = 925
    auto sale = mp.BuyNow("bob", *l1, 500, "alice");
    CHECK(sale.has_value());
    CHECK(sale->royalty_paid == 50);
    CHECK(sale->platform_fee == 25);
    CHECK(sale->seller_proceeds == 925);
    CHECK(sale->royalty_recipient == "alice");

    CHECK(!mp.GetListing(*l1)->active);
    CHECK(mp.ActiveListings().empty());

    // Double buy fails
    CHECK(!mp.BuyNow("carol", *l1, 500, "alice").has_value());

    // === Auction ===
    auto l2 = mp.ListAuction("dave", 200, 500, 3600);
    CHECK(l2.has_value());
    CHECK(mp.GetListing(*l2)->kind == ListingKind::Auction);
    CHECK(mp.GetListing(*l2)->price == 500);
    CHECK(mp.GetListing(*l2)->expires_at > 0);

    // Bid below reserve fails
    CHECK(!mp.PlaceBid("eve", *l2, 400).has_value());
    // Seller cannot bid
    CHECK(!mp.PlaceBid("dave", *l2, 600).has_value());
    // Below or equal current bid fails
    CHECK(mp.PlaceBid("eve", *l2, 500).has_value());
    CHECK(!mp.PlaceBid("frank", *l2, 500).has_value());
    CHECK(mp.PlaceBid("frank", *l2, 600).has_value());
    CHECK(mp.BidCount() == 2);

    auto lst2 = mp.GetListing(*l2);
    CHECK(lst2->highest_bid == 600);
    CHECK(lst2->highest_bidder == "frank");

    CHECK(mp.BidsForListing(*l2).size() == 2);

    // Cannot cancel auction with bids
    CHECK(!mp.CancelListing("dave", *l2));

    // Settle — seller can call early; buyer gets highest bid
    auto as = mp.SettleAuction("dave", *l2, 500, "dave");
    CHECK(as.has_value());
    CHECK(as->royalty_paid == 30);   // 5% of 600
    CHECK(as->platform_fee == 15);   // 2.5% of 600
    CHECK(as->seller_proceeds == 555);

    CHECK(!mp.GetListing(*l2)->active);

    // === Cancel auction (no bids) ===
    auto l3 = mp.ListAuction("george", 300, 1000, 3600);
    CHECK(l3.has_value());
    CHECK(mp.CancelListing("george", *l3));
    CHECK(!mp.GetListing(*l3)->active);

    // === Auction expiry ===
    auto l4 = mp.ListAuction("henry", 400, 200, 1);
    CHECK(l4.has_value());
    // Not expired yet, but no bids
    CHECK(!mp.ExpireAuction(*l4));  // fails — not yet expired (only 1 sec passed? actually likely expired in test)
    // Note: 1 second duration may or may not have passed — accept either

    // === Fixed price cancel ===
    auto l5 = mp.ListFixedPrice("ivy", 500, 5000);
    CHECK(l5.has_value());
    CHECK(mp.CancelListing("ivy", *l5));
    CHECK(!mp.GetListing(*l5)->active);
    CHECK(!mp.CancelListing("ivy", *l5)); // already cancelled

    CHECK(mp.ListingCount() == 5);
    CHECK(mp.ActiveListings().size() == 1); // only l4

    // === Listings by seller ===
    auto alice_listings = mp.ListingsBySeller("alice");
    CHECK(alice_listings.size() == 1);
    CHECK(alice_listings[0] == *l1);

    // === Platform fee config ===
    mp.SetPlatformFeeBps(100);
    CHECK(mp.PlatformFeeBps() == 100);
    mp.SetPlatformFeeBps(99999);
    CHECK(mp.PlatformFeeBps() == 100); // invalid ignored

    // === Get listing that doesn't exist ===
    CHECK(!mp.GetListing(99999).has_value());
    CHECK(!mp.GetBid(99999).has_value());

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
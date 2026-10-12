#include "shivrai/nft_platform/nft_platform.h"
#include <cassert>
#include <iostream>

using namespace shivrai::nft_platform;

int main() {
    int passed = 0;
    auto CHECK = [&](bool c) { assert(c); ++passed; };

    NFTPlatform nft;

    CHECK(nft.CollectionCount() == 0);
    CHECK(nft.TotalTokens() == 0);

    auto cid = nft.CreateCollection("alice", "Shivrai Punks", "SPUNK", 500, 3, "ipfs://base/");
    CHECK(cid.has_value());
    CHECK(nft.CollectionCount() == 1);

    auto col = nft.GetCollection(*cid);
    CHECK(col.has_value());
    CHECK(col->creator == "alice");
    CHECK(col->name == "Shivrai Punks");
    CHECK(col->symbol == "SPUNK");
    CHECK(col->royalty_bps == 500);
    CHECK(col->max_supply == 3);
    CHECK(col->minted == 0);

    CHECK(!nft.CreateCollection("", "X", "Y", 0, 0, "").has_value());
    CHECK(!nft.CreateCollection("bob", "X", "Y", 9999, 0, "").has_value());

    auto t1 = nft.Mint(*cid, "alice", "ipfs://1");
    auto t2 = nft.Mint(*cid, "alice", "ipfs://2");
    auto t3 = nft.Mint(*cid, "bob",   "ipfs://3");
    CHECK(t1.has_value() && t2.has_value() && t3.has_value());
    CHECK(nft.TotalTokens() == 3);
    CHECK(nft.TotalSupply(*cid) == 3);
    CHECK(nft.OwnerOf(*t1) == "alice");
    CHECK(nft.OwnerOf(*t3) == "bob");

    auto tt = nft.GetToken(*t1);
    CHECK(tt.has_value());
    CHECK(tt->token_id == 0);
    CHECK(tt->collection == *cid);
    CHECK(tt->token_uri == "ipfs://1");

    CHECK(nft.TokensOf("alice").size() == 2);
    CHECK(nft.TokensOf("bob").size() == 1);
    CHECK(nft.TokensInCollection(*cid).size() == 3);

    CHECK(!nft.Mint(*cid, "carol", "ipfs://4").has_value());

    CHECK(nft.Transfer("alice", "bob", *t1));
    CHECK(nft.OwnerOf(*t1) == "bob");
    CHECK(nft.TokensOf("alice").size() == 1);
    CHECK(nft.TokensOf("bob").size() == 2);
    CHECK(!nft.Transfer("alice", "carol", *t1));

    CHECK(nft.Approve("bob", "carol", *t1));
    auto ap = nft.GetApproved(*t1);
    CHECK(ap.has_value() && *ap == "carol");

    CHECK(nft.TransferFrom("carol", "bob", "carol", *t1));
    CHECK(nft.OwnerOf(*t1) == "carol");
    CHECK(!nft.GetApproved(*t1).has_value());

    CHECK(!nft.TransferFrom("dave", "carol", "dave", *t1));

    CHECK(nft.SetApprovalForAll("carol", "operator1", true));
    CHECK(nft.IsApprovedForAll("carol", "operator1"));
    CHECK(nft.TransferFrom("operator1", "carol", "alice", *t1));
    CHECK(nft.OwnerOf(*t1) == "alice");
    CHECK(nft.SetApprovalForAll("carol", "operator1", false));
    CHECK(!nft.IsApprovedForAll("carol", "operator1"));

    CHECK(nft.Burn("bob", *t3));
    CHECK(nft.OwnerOf(*t3).empty());
    CHECK(nft.TotalTokens() == 2);
    CHECK(nft.TotalSupply(*cid) == 2);
    CHECK(!nft.Burn("bob", *t3));

    auto sale = nft.RecordSale(*t2, 1000);
    CHECK(sale.has_value());
    CHECK(sale->royalty_paid == 50);
    CHECK(sale->seller_proceeds == 950);
    CHECK(sale->royalty_recipient == "alice");

    auto bad = nft.RecordSale(99999, 1000);
    CHECK(!bad.has_value());

    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}
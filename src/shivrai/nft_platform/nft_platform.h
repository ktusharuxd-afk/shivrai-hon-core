#ifndef SHIVRAI_HON_NFT_PLATFORM_H
#define SHIVRAI_HON_NFT_PLATFORM_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace shivrai::nft_platform {

using CollectionId = uint64_t;
using TokenId      = uint64_t;
using GlobalId     = uint64_t;

static constexpr uint32_t MAX_ROYALTY_BPS = 2000;

struct Collection {
    CollectionId id{0};
    std::string  creator;
    std::string  name;
    std::string  symbol;
    uint32_t     royalty_bps{0};
    uint64_t     max_supply{0};
    uint64_t     minted{0};
    std::string  base_uri;
};

struct Token {
    GlobalId     gid{0};
    CollectionId collection{0};
    TokenId      token_id{0};
    std::string  owner;
    std::string  token_uri;
};

struct SaleResult {
    uint64_t    seller_proceeds{0};
    uint64_t    royalty_paid{0};
    std::string royalty_recipient;
};

class NFTPlatform {
public:
    NFTPlatform();

    std::optional<CollectionId> CreateCollection(const std::string& creator,
                                                 const std::string& name,
                                                 const std::string& symbol,
                                                 uint32_t royalty_bps,
                                                 uint64_t max_supply,
                                                 const std::string& base_uri);
    std::optional<Collection> GetCollection(CollectionId id) const;
    size_t CollectionCount() const;

    std::optional<GlobalId> Mint(CollectionId cid,
                                 const std::string& to,
                                 const std::string& token_uri);

    bool Transfer(const std::string& from,
                  const std::string& to,
                  GlobalId gid);
    bool Approve(const std::string& owner,
                 const std::string& approved,
                 GlobalId gid);
    bool SetApprovalForAll(const std::string& owner,
                           const std::string& oper,
                           bool approved);
    bool TransferFrom(const std::string& spender,
                      const std::string& from,
                      const std::string& to,
                      GlobalId gid);
    bool Burn(const std::string& owner, GlobalId gid);

    std::optional<Token>        GetToken(GlobalId gid) const;
    std::string                 OwnerOf(GlobalId gid) const;
    std::vector<GlobalId>       TokensOf(const std::string& owner) const;
    std::vector<GlobalId>       TokensInCollection(CollectionId cid) const;
    uint64_t                    TotalSupply(CollectionId cid) const;
    size_t                      TotalTokens() const;
    std::optional<std::string>  GetApproved(GlobalId gid) const;
    bool                        IsApprovedForAll(const std::string& owner,
                                                 const std::string& oper) const;

    std::optional<SaleResult> RecordSale(GlobalId gid, uint64_t sale_price);

private:
    void RemoveFromOwnerIndex(const std::string& owner, GlobalId gid);

    mutable std::recursive_mutex cs_;
    std::map<CollectionId, Collection> collections_;
    std::map<GlobalId, Token>          tokens_;
    std::map<CollectionId, std::set<GlobalId>> collection_tokens_;
    std::map<std::string, std::set<GlobalId>>  owner_tokens_;
    std::map<GlobalId, std::string>    approvals_;
    std::map<std::string, std::set<std::string>> operator_approvals_;

    CollectionId  next_cid_{1};
    GlobalId      next_gid_{1};
    std::map<CollectionId, TokenId> next_token_id_;
};

}
#endif
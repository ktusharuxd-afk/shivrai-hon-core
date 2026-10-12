#ifndef SHIVRAI_HON_WRAPPED_TOKENS_H
#define SHIVRAI_HON_WRAPPED_TOKENS_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::wrapped_tokens {
struct WrappedAsset {
    std::string symbol;
    std::string underlying_chain;
    std::string underlying_symbol;
    uint64_t    total_supply{0};
    uint64_t    total_backing{0};
    bool        active{true};
};
class WrappedTokens {
public:
    WrappedTokens();
    bool Register(const std::string& symbol, const std::string& chain, const std::string& underlying);
    bool SetActive(const std::string& symbol, bool active);
    std::optional<WrappedAsset> Get(const std::string& symbol) const;
    bool Wrap(const std::string& symbol, const std::string& user, uint64_t amount);
    std::optional<uint64_t> Unwrap(const std::string& symbol, const std::string& user, uint64_t amount);
    uint64_t BalanceOf(const std::string& symbol, const std::string& user) const;
    uint64_t TotalSupply(const std::string& symbol) const;
    uint64_t TotalBacking(const std::string& symbol) const;
    bool IsFullyBacked(const std::string& symbol) const;
    size_t AssetCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, WrappedAsset> assets_;
    std::map<std::string, std::map<std::string, uint64_t>> balances_;
};
}
#endif

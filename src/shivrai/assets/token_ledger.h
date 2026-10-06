#ifndef SHIVRAI_HON_TOKEN_LEDGER_H
#define SHIVRAI_HON_TOKEN_LEDGER_H
#include <utility>
#include <cstdint>
#include <string>
#include <unordered_map>
namespace shivrai::assets {
class TokenLedger {
public:
 explicit TokenLedger(std::string symbol, uint8_t decimals=18):symbol_(std::move(symbol)),decimals_(decimals){}
 bool mint(const std::string& to,uint64_t amount); bool transfer(const std::string& from,const std::string& to,uint64_t amount);
 uint64_t balance(const std::string& who) const; const std::string& symbol() const{return symbol_;}
private: std::string symbol_; uint8_t decimals_; std::unordered_map<std::string,uint64_t> balances_;
};
}
#endif

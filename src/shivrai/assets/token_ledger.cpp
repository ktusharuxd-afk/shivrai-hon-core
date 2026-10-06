#include "token_ledger.h"
#include <limits>
namespace shivrai::assets {
bool TokenLedger::mint(const std::string& to,uint64_t n){auto& b=balances_[to]; if(n>std::numeric_limits<uint64_t>::max()-b)return false;b+=n;return true;}
bool TokenLedger::transfer(const std::string& f,const std::string& t,uint64_t n){if(balance(f)<n)return false;balances_[f]-=n;balances_[t]+=n;return true;}
uint64_t TokenLedger::balance(const std::string& w) const {auto it=balances_.find(w);return it==balances_.end()?0:it->second;}
}

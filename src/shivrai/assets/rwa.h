#ifndef SHIVRAI_HON_RWA_H
#define SHIVRAI_HON_RWA_H
#include <string>
#include <unordered_map>
namespace shivrai::assets { struct RWA {std::string id,issuer,metadata_hash,jurisdiction;bool verified{false};}; class RWARegistry{public:bool register_asset(RWA a);bool verify(const std::string&id);const RWA* get(const std::string&id)const;private:std::unordered_map<std::string,RWA> m_;}; }
#endif

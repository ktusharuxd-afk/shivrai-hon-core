#ifndef SHIVRAI_HON_GAMING_ASSETS_H
#define SHIVRAI_HON_GAMING_ASSETS_H
#include <string>
#include <unordered_map>
namespace shivrai::gaming {
struct Asset {std::string id;std::string owner;std::string metadata_hash;bool transferable{true};};
class Registry {public:bool mint(Asset a);bool transfer(const std::string&id,const std::string&to);const Asset* get(const std::string&id)const;private:std::unordered_map<std::string,Asset> a_;};
}
#endif

#include "rwa.h"
#include <utility>
namespace shivrai::assets {bool RWARegistry::register_asset(RWA a){if(a.id.empty()||a.issuer.empty()||a.metadata_hash.empty()||m_.count(a.id))return false;m_.emplace(a.id,std::move(a));return true;}bool RWARegistry::verify(const std::string&i){auto it=m_.find(i);if(it==m_.end())return false;it->second.verified=true;return true;}const RWA* RWARegistry::get(const std::string&i)const{auto it=m_.find(i);return it==m_.end()?nullptr:&it->second;}}

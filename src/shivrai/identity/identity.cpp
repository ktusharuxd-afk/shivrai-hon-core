#include "identity.h"
#include <utility>
namespace shivrai::identity {
bool Registry::register_identity(IdentityRecord r){if(r.subject.empty()||r.controller.empty()||records_.count(r.subject))return false;records_.emplace(r.subject,std::move(r));return true;}
bool Registry::revoke(const std::string&s){auto it=records_.find(s);if(it==records_.end())return false;it->second.active=false;return true;}
const IdentityRecord* Registry::get(const std::string&s)const{auto it=records_.find(s);return it==records_.end()?nullptr:&it->second;}
}

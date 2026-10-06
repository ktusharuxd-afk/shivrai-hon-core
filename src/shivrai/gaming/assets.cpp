#include "assets.h"
#include <utility>
namespace shivrai::gaming {bool Registry::mint(Asset x){if(x.id.empty()||x.owner.empty()||a_.count(x.id))return false;a_.emplace(x.id,std::move(x));return true;} bool Registry::transfer(const std::string&i,const std::string&t){auto it=a_.find(i);if(it==a_.end()||!it->second.transferable||t.empty())return false;it->second.owner=t;return true;} const Asset* Registry::get(const std::string&i)const{auto it=a_.find(i);return it==a_.end()?nullptr:&it->second;}}

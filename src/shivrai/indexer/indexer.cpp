#include "indexer.h"
#include <ctime>
namespace shivrai::indexer {
Indexer::Indexer() = default;
uint64_t Indexer::Index(const std::string& cat, const std::string& k, const std::string& v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (cat.empty() || k.empty()) return 0;
    uint64_t id = next_id_++;
    IndexedItem it;
    it.id = id; it.category = cat; it.key = k; it.value = v;
    it.ts = (int64_t)std::time(nullptr);
    items_[id] = it;
    by_category_[cat].insert(id);
    by_key_[{cat, k}].insert(id);
    return id;
}
std::vector<IndexedItem> Indexer::Lookup(const std::string& cat, const std::string& k) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<IndexedItem> out;
    auto it = by_key_.find({cat, k});
    if (it == by_key_.end()) return out;
    for (auto id : it->second) out.push_back(items_.at(id));
    return out;
}
std::vector<IndexedItem> Indexer::ByCategory(const std::string& cat) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<IndexedItem> out;
    auto it = by_category_.find(cat);
    if (it == by_category_.end()) return out;
    for (auto id : it->second) out.push_back(items_.at(id));
    return out;
}
std::optional<IndexedItem> Indexer::Get(uint64_t id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = items_.find(id);
    if (it == items_.end()) return std::nullopt;
    return it->second;
}
std::vector<uint64_t> Indexer::Categories() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<uint64_t> out; // dummy — not used, kept for API
    (void)out;
    std::vector<uint64_t> r;
    return r;
}
std::vector<std::string> Indexer::KeysIn(const std::string& cat) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::set<std::string> ks;
    for (auto& [k, ids] : by_key_) if (k.first == cat) ks.insert(k.second);
    return std::vector<std::string>(ks.begin(), ks.end());
}
size_t Indexer::Count() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return items_.size();
}
bool Indexer::Remove(uint64_t id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = items_.find(id);
    if (it == items_.end()) return false;
    by_category_[it->second.category].erase(id);
    by_key_[{it->second.category, it->second.key}].erase(id);
    items_.erase(it);
    return true;
}
}

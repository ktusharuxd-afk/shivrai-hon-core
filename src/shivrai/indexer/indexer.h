#ifndef SHIVRAI_HON_INDEXER_H
#define SHIVRAI_HON_INDEXER_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::indexer {
struct IndexedItem {
    uint64_t id{0};
    std::string category;
    std::string key;
    std::string value;
    int64_t ts{0};
};
class Indexer {
public:
    Indexer();
    uint64_t Index(const std::string& category, const std::string& key, const std::string& value);
    std::vector<IndexedItem> Lookup(const std::string& category, const std::string& key) const;
    std::vector<IndexedItem> ByCategory(const std::string& category) const;
    std::optional<IndexedItem> Get(uint64_t id) const;
    std::vector<uint64_t> Categories() const;
    std::vector<std::string> KeysIn(const std::string& category) const;
    size_t Count() const;
    bool Remove(uint64_t id);
private:
    mutable std::recursive_mutex cs_;
    std::map<uint64_t, IndexedItem> items_;
    std::map<std::string, std::set<uint64_t>> by_category_;
    std::map<std::pair<std::string, std::string>, std::set<uint64_t>> by_key_;
    uint64_t next_id_{1};
};
}
#endif

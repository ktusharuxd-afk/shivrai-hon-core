#include "analytics.h"
#include <algorithm>
#include <ctime>
namespace shivrai::analytics {
Analytics::Analytics() = default;
bool Analytics::Record(const std::string& s, uint64_t v) {
    return RecordAt(s, (int64_t)std::time(nullptr), v);
}
bool Analytics::RecordAt(const std::string& s, int64_t ts, uint64_t v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty()) return false;
    data_[s].name = s;
    data_[s].points.emplace_back(ts, v);
    return true;
}
uint64_t Analytics::Sum(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end()) return 0;
    uint64_t t = 0;
    for (auto& p : it->second.points) t += p.second;
    return t;
}
uint64_t Analytics::Count(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    return it == data_.end() ? 0 : it->second.points.size();
}
uint64_t Analytics::Max(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end() || it->second.points.empty()) return 0;
    uint64_t m = 0;
    for (auto& p : it->second.points) m = std::max(m, p.second);
    return m;
}
uint64_t Analytics::Min(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end() || it->second.points.empty()) return 0;
    uint64_t m = UINT64_MAX;
    for (auto& p : it->second.points) m = std::min(m, p.second);
    return m;
}
double Analytics::Avg(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    if (it == data_.end() || it->second.points.empty()) return 0.0;
    uint64_t sum = 0;
    for (auto& p : it->second.points) sum += p.second;
    return (double)sum / it->second.points.size();
}
std::vector<std::pair<int64_t,uint64_t>> Analytics::Points(const std::string& s) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = data_.find(s);
    return it == data_.end() ? std::vector<std::pair<int64_t,uint64_t>>{} : it->second.points;
}
std::vector<std::string> Analytics::AllSeries() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<std::string> out;
    for (auto& [k, v] : data_) out.push_back(k);
    return out;
}
size_t Analytics::SeriesCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return data_.size();
}
bool Analytics::Clear(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return data_.erase(s) > 0;
}
}

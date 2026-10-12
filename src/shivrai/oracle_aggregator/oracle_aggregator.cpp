#include "oracle_aggregator.h"
#include <algorithm>
#include <ctime>
namespace shivrai::oracle_aggregator {
OracleAggregator::OracleAggregator() = default;
bool OracleAggregator::RegisterSource(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || sources_.count(s)) return false;
    sources_[s] = true;
    return true;
}
bool OracleAggregator::RemoveSource(const std::string& s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return sources_.erase(s) > 0;
}
bool OracleAggregator::SubmitPrice(const std::string& s, const std::string& a, uint64_t p) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s.empty() || a.empty() || p == 0) return false;
    if (!sources_.count(s)) return false;
    PricePoint pp;
    pp.source = s; pp.price = p;
    pp.ts = (int64_t)std::time(nullptr);
    latest_[a][s] = pp;
    return true;
}
std::optional<AggregatedPrice> OracleAggregator::Aggregate(const std::string& a) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = latest_.find(a);
    if (it == latest_.end()) return std::nullopt;
    AggregatedPrice ap;
    ap.asset = a;
    int64_t now = (int64_t)std::time(nullptr);
    std::vector<uint64_t> prices;
    for (auto& [src, pp] : it->second) {
        if (now - pp.ts > max_age_sec_) continue;
        if (!sources_.count(src)) continue;
        prices.push_back(pp.price);
    }
    if (prices.empty()) return std::nullopt;
    std::sort(prices.begin(), prices.end());
    ap.median = prices[prices.size() / 2];
    ap.min_price = prices.front();
    ap.max_price = prices.back();
    ap.sources = prices.size();
    ap.ts = now;
    ap.valid = true;
    return ap;
}
size_t OracleAggregator::SourceCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return sources_.size();
}
size_t OracleAggregator::PendingCount(const std::string& a) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = latest_.find(a);
    return it == latest_.end() ? 0 : it->second.size();
}
void OracleAggregator::SetMaxAgeSec(int64_t s) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (s > 0) max_age_sec_ = s;
}
int64_t OracleAggregator::MaxAgeSec() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return max_age_sec_;
}
}

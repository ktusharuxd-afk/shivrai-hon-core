#include "shivrai/oracle/oracle.h"

#include <algorithm>
#include <chrono>

namespace shivrai::oracle {

static int64_t now_sec() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

OracleRegistry::OracleRegistry() = default;

// ---------------- Oracle management ----------------

bool OracleRegistry::RegisterOracle(const std::string& oracle_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (oracle_id.empty()) return false;
    return authorized_.insert(oracle_id).second;
}

bool OracleRegistry::UnregisterOracle(const std::string& oracle_id) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return authorized_.erase(oracle_id) > 0;
}

bool OracleRegistry::IsAuthorized(const std::string& oracle_id) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return authorized_.count(oracle_id) > 0;
}

std::vector<std::string> OracleRegistry::ListOracles() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return std::vector<std::string>(authorized_.begin(), authorized_.end());
}

// ---------------- Internal helpers ----------------

uint64_t OracleRegistry::ComputeMedian(const PerAsset& pa) const {
    if (pa.latest_by_reporter.empty()) return 0;
    std::vector<uint64_t> prices;
    prices.reserve(pa.latest_by_reporter.size());
    for (const auto& kv : pa.latest_by_reporter) prices.push_back(kv.second.price);
    std::sort(prices.begin(), prices.end());
    size_t n = prices.size();
    if (n % 2 == 1) return prices[n / 2];
    // Even count: average of two middles
    return (prices[n/2 - 1] + prices[n/2]) / 2;
}

// ---------------- Price submission ----------------

bool OracleRegistry::SubmitPrice(const std::string& asset,
                                  uint64_t price,
                                  int64_t timestamp,
                                  const std::string& reporter) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (paused_) return false;
    if (asset.empty() || reporter.empty()) return false;
    if (price == 0) return false;
    if (!authorized_.count(reporter)) return false;

    // Timestamp must not be in the future
    int64_t now = now_sec();
    if (timestamp > now + 60) return false;

    PerAsset& pa = feeds_[asset];

    // Deviation check: compare against current median (if enough sources)
    if (pa.latest_by_reporter.size() >= min_sources_) {
        uint64_t med = ComputeMedian(pa);
        if (med > 0) {
            uint64_t diff = (price > med) ? (price - med) : (med - price);
            // diff / med * 10000 <= max_deviation_bps
            uint64_t allowed = (med * max_deviation_bps_) / 10000;
            if (diff > allowed) return false;
        }
    }

    // Reject older prices from same reporter
    auto it = pa.latest_by_reporter.find(reporter);
    if (it != pa.latest_by_reporter.end() && it->second.timestamp > timestamp) {
        return false;
    }

    PricePoint p;
    p.asset = asset;
    p.price = price;
    p.timestamp = timestamp;
    p.reporter = reporter;
    pa.latest_by_reporter[reporter] = p;

    // History ring buffer
    pa.history.push_back(p);
    while (pa.history.size() > history_size_) pa.history.pop_front();

    ++total_updates_;
    return true;
}

// ---------------- Price query ----------------

std::optional<uint64_t> OracleRegistry::GetPrice(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = feeds_.find(asset);
    if (it == feeds_.end()) return std::nullopt;
    if (it->second.latest_by_reporter.size() < min_sources_) return std::nullopt;
    return ComputeMedian(it->second);
}

std::optional<PriceFeed> OracleRegistry::GetFeed(const std::string& asset) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = feeds_.find(asset);
    if (it == feeds_.end()) return std::nullopt;

    PriceFeed f;
    f.asset = asset;
    f.source_count = it->second.latest_by_reporter.size();
    if (f.source_count < min_sources_) {
        f.is_stale = true;
        return f;
    }
    f.median_price = ComputeMedian(it->second);
    int64_t latest = 0;
    for (const auto& kv : it->second.latest_by_reporter) {
        if (kv.second.timestamp > latest) latest = kv.second.timestamp;
    }
    f.last_update = latest;
    f.is_stale = (now_sec() - latest) > heartbeat_seconds_;
    return f;
}

std::vector<PriceFeed> OracleRegistry::ListFeeds() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<PriceFeed> out;
    for (const auto& kv : feeds_) {
        auto f = GetFeed(kv.first);
        if (f) out.push_back(*f);
    }
    return out;
}

std::vector<PricePoint> OracleRegistry::GetHistory(const std::string& asset, size_t limit) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    std::vector<PricePoint> out;
    auto it = feeds_.find(asset);
    if (it == feeds_.end()) return out;
    size_t n = it->second.history.size();
    size_t start = (limit == 0 || limit >= n) ? 0 : (n - limit);
    for (size_t i = start; i < n; ++i) out.push_back(it->second.history[i]);
    return out;
}

bool OracleRegistry::IsStale(const std::string& asset, int64_t now) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = feeds_.find(asset);
    if (it == feeds_.end()) return true;
    if (it->second.latest_by_reporter.size() < min_sources_) return true;
    int64_t latest = 0;
    for (const auto& kv : it->second.latest_by_reporter) {
        if (kv.second.timestamp > latest) latest = kv.second.timestamp;
    }
    return (now - latest) > heartbeat_seconds_;
}

// ---------------- Emergency ----------------

void OracleRegistry::Pause()  { std::lock_guard<std::recursive_mutex> lk(cs_); paused_ = true; }
void OracleRegistry::Resume() { std::lock_guard<std::recursive_mutex> lk(cs_); paused_ = false; }
bool OracleRegistry::IsPaused() const { std::lock_guard<std::recursive_mutex> lk(cs_); return paused_; }

// ---------------- Maintenance ----------------

void OracleRegistry::PruneStale(int64_t now) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    for (auto& kv : feeds_) {
        auto& pa = kv.second;
        // Remove reporters whose price is older than 2x heartbeat
        for (auto it = pa.latest_by_reporter.begin(); it != pa.latest_by_reporter.end(); ) {
            if ((now - it->second.timestamp) > 2 * heartbeat_seconds_) {
                it = pa.latest_by_reporter.erase(it);
            } else {
                ++it;
            }
        }
    }
}

// ---------------- Diagnostics ----------------

size_t OracleRegistry::FeedCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return feeds_.size();
}

size_t OracleRegistry::TotalUpdates() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return (size_t)total_updates_;
}

}  // namespace shivrai::oracle

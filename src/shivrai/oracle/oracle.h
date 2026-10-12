#ifndef SHIVRAI_HON_ORACLE_H
#define SHIVRAI_HON_ORACLE_H

#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace shivrai::oracle {

// Defaults
static constexpr int      DEFAULT_HEARTBEAT_SECONDS = 300;   // 5 min
static constexpr size_t   DEFAULT_HISTORY_SIZE      = 1000;
static constexpr uint32_t DEFAULT_MAX_DEVIATION_BPS = 2000;  // 20%
static constexpr size_t   DEFAULT_MIN_SOURCES       = 3;

struct PricePoint {
    std::string asset;
    uint64_t    price{0};          // smallest units (e.g. satoshis)
    int64_t     timestamp{0};
    std::string reporter;          // oracle id that signed
};

struct PriceFeed {
    std::string asset;
    uint64_t    median_price{0};
    int64_t     last_update{0};
    size_t      source_count{0};
    bool        is_stale{true};
};

class OracleRegistry {
public:
    OracleRegistry();

    // Config
    void SetHeartbeatSeconds(int s)              { heartbeat_seconds_ = s; }
    void SetMaxDeviationBps(uint32_t bps)        { max_deviation_bps_ = bps; }
    void SetMinSources(size_t n)                 { min_sources_ = n; }
    void SetHistorySize(size_t n)                { history_size_ = n; }
    int  HeartbeatSeconds() const                { return heartbeat_seconds_; }
    uint32_t MaxDeviationBps() const             { return max_deviation_bps_; }

    // Oracle management
    bool RegisterOracle(const std::string& oracle_id);
    bool UnregisterOracle(const std::string& oracle_id);
    bool IsAuthorized(const std::string& oracle_id) const;
    std::vector<std::string> ListOracles() const;

    // Price submission
    // Returns false if: unauthorized, stale signature, deviation too high, or paused
    bool SubmitPrice(const std::string& asset,
                     uint64_t price,
                     int64_t timestamp,
                     const std::string& reporter);

    // Price query
    std::optional<uint64_t> GetPrice(const std::string& asset) const;
    std::optional<PriceFeed> GetFeed(const std::string& asset) const;
    std::vector<PriceFeed>   ListFeeds() const;
    std::vector<PricePoint>  GetHistory(const std::string& asset, size_t limit = 0) const;
    bool IsStale(const std::string& asset, int64_t now) const;

    // Emergency
    void Pause();
    void Resume();
    bool IsPaused() const;

    // Maintenance
    void PruneStale(int64_t now);

    // Diagnostics
    size_t FeedCount() const;
    size_t TotalUpdates() const;

private:
    struct PerAsset {
        std::deque<PricePoint> history;
        std::map<std::string, PricePoint> latest_by_reporter;
    };

    uint64_t ComputeMedian(const PerAsset& pa) const;

    mutable std::recursive_mutex cs_;
    std::set<std::string> authorized_;
    std::map<std::string, PerAsset> feeds_;

    int      heartbeat_seconds_{DEFAULT_HEARTBEAT_SECONDS};
    uint32_t max_deviation_bps_{DEFAULT_MAX_DEVIATION_BPS};
    size_t   min_sources_{DEFAULT_MIN_SOURCES};
    size_t   history_size_{DEFAULT_HISTORY_SIZE};
    bool     paused_{false};
    uint64_t total_updates_{0};
};

}  // namespace shivrai::oracle

#endif

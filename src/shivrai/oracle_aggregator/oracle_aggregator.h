#ifndef SHIVRAI_HON_ORACLE_AGGREGATOR_H
#define SHIVRAI_HON_ORACLE_AGGREGATOR_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <vector>
namespace shivrai::oracle_aggregator {
struct PricePoint {
    std::string source;
    uint64_t price{0};
    int64_t ts{0};
};
struct AggregatedPrice {
    std::string asset;
    uint64_t median{0};
    uint64_t min_price{0};
    uint64_t max_price{0};
    size_t sources{0};
    int64_t ts{0};
    bool valid{false};
};
class OracleAggregator {
public:
    OracleAggregator();
    bool RegisterSource(const std::string& source);
    bool SubmitPrice(const std::string& source, const std::string& asset, uint64_t price);
    std::optional<AggregatedPrice> Aggregate(const std::string& asset) const;
    size_t SourceCount() const;
    size_t PendingCount(const std::string& asset) const;
    bool RemoveSource(const std::string& source);
    void SetMaxAgeSec(int64_t sec);
    int64_t MaxAgeSec() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, bool> sources_;
    std::map<std::string, std::map<std::string, PricePoint>> latest_;
    int64_t max_age_sec_{300};
};
}
#endif

#ifndef SHIVRAI_HON_ANALYTICS_H
#define SHIVRAI_HON_ANALYTICS_H
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>
namespace shivrai::analytics {
struct Series { std::string name; std::vector<std::pair<int64_t,uint64_t>> points; };
class Analytics {
public:
    Analytics();
    bool Record(const std::string& series, uint64_t value);
    bool RecordAt(const std::string& series, int64_t ts, uint64_t value);
    uint64_t Sum(const std::string& series) const;
    uint64_t Count(const std::string& series) const;
    uint64_t Max(const std::string& series) const;
    uint64_t Min(const std::string& series) const;
    double Avg(const std::string& series) const;
    std::vector<std::pair<int64_t,uint64_t>> Points(const std::string& series) const;
    std::vector<std::string> AllSeries() const;
    size_t SeriesCount() const;
    bool Clear(const std::string& series);
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, Series> data_;
};
}
#endif

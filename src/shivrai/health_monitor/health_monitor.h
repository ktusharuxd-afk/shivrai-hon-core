#ifndef SHIVRAI_HON_HEALTH_MONITOR_H
#define SHIVRAI_HON_HEALTH_MONITOR_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::health_monitor {
enum class Status { Healthy, Degraded, Paused };
struct Metric {
    std::string name;
    uint64_t value{0};
    int64_t ts{0};
};
class HealthMonitor {
public:
    HealthMonitor();
    bool RegisterMetric(const std::string& name, uint64_t threshold, bool high_is_bad);
    bool Report(const std::string& name, uint64_t value);
    std::optional<Metric> GetMetric(const std::string& name) const;
    Status StatusOf(const std::string& name) const;
    bool IsHealthy(const std::string& name) const;
    bool Pause(const std::string& reason);
    bool Resume();
    bool IsPaused() const;
    std::string PauseReason() const;
    size_t MetricCount() const;
    size_t TriggeredCount() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<std::string, uint64_t> thresholds_;
    std::map<std::string, bool> high_bad_;
    std::map<std::string, Metric> metrics_;
    bool paused_{false};
    std::string pause_reason_;
};
}
#endif

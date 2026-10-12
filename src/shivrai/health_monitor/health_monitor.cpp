#include "health_monitor.h"
#include <ctime>
namespace shivrai::health_monitor {
HealthMonitor::HealthMonitor() = default;
bool HealthMonitor::RegisterMetric(const std::string& n, uint64_t t, bool hib) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (n.empty() || thresholds_.count(n)) return false;
    thresholds_[n] = t;
    high_bad_[n] = hib;
    return true;
}
bool HealthMonitor::Report(const std::string& n, uint64_t v) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!thresholds_.count(n)) return false;
    Metric m;
    m.name = n; m.value = v;
    m.ts = (int64_t)std::time(nullptr);
    metrics_[n] = m;
    return true;
}
std::optional<Metric> HealthMonitor::GetMetric(const std::string& n) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    auto it = metrics_.find(n);
    if (it == metrics_.end()) return std::nullopt;
    return it->second;
}
Status HealthMonitor::StatusOf(const std::string& n) const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (paused_) return Status::Paused;
    auto mit = metrics_.find(n);
    if (mit == metrics_.end()) return Status::Healthy;
    uint64_t t = thresholds_.at(n);
    bool hib = high_bad_.at(n);
    if (hib && mit->second.value > t) return Status::Degraded;
    if (!hib && mit->second.value < t) return Status::Degraded;
    return Status::Healthy;
}
bool HealthMonitor::IsHealthy(const std::string& n) const {
    return StatusOf(n) == Status::Healthy;
}
bool HealthMonitor::Pause(const std::string& r) {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (paused_) return false;
    paused_ = true;
    pause_reason_ = r;
    return true;
}
bool HealthMonitor::Resume() {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    if (!paused_) return false;
    paused_ = false;
    pause_reason_.clear();
    return true;
}
bool HealthMonitor::IsPaused() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return paused_;
}
std::string HealthMonitor::PauseReason() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return pause_reason_;
}
size_t HealthMonitor::MetricCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    return thresholds_.size();
}
size_t HealthMonitor::TriggeredCount() const {
    std::lock_guard<std::recursive_mutex> lk(cs_);
    size_t n = 0;
    for (auto& [k, v] : metrics_) {
        if (StatusOf(k) != Status::Healthy) ++n;
    }
    return n;
}
}

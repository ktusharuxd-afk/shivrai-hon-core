#include "shivrai/health_monitor/health_monitor.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::health_monitor;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    HealthMonitor hm;
    CHECK(hm.MetricCount() == 0);
    CHECK(!hm.IsPaused());
    CHECK(!hm.RegisterMetric("", 100, true));
    CHECK(hm.RegisterMetric("latency", 1000, true));
    CHECK(hm.RegisterMetric("uptime", 95, false));
    CHECK(!hm.RegisterMetric("latency", 1000, true));
    CHECK(hm.MetricCount() == 2);
    CHECK(!hm.Report("unknown", 50));
    CHECK(hm.Report("latency", 500));
    CHECK(hm.Report("uptime", 99));
    CHECK(hm.IsHealthy("latency"));
    CHECK(hm.IsHealthy("uptime"));
    CHECK(hm.StatusOf("latency") == Status::Healthy);
    CHECK(hm.Report("latency", 2000));
    CHECK(hm.StatusOf("latency") == Status::Degraded);
    CHECK(!hm.IsHealthy("latency"));
    CHECK(hm.Report("uptime", 90));
    CHECK(hm.StatusOf("uptime") == Status::Degraded);
    CHECK(hm.TriggeredCount() == 2);
    CHECK(hm.Report("latency", 500));
    CHECK(hm.Report("uptime", 99));
    CHECK(hm.TriggeredCount() == 0);
    auto m = hm.GetMetric("latency");
    CHECK(m.has_value());
    CHECK(m->name == "latency");
    CHECK(m->value == 500);
    CHECK(!hm.GetMetric("unknown").has_value());
    CHECK(hm.Pause("emergency"));
    CHECK(hm.IsPaused());
    CHECK(hm.PauseReason() == "emergency");
    CHECK(hm.StatusOf("latency") == Status::Paused);
    CHECK(!hm.Pause("again"));
    CHECK(hm.Resume());
    CHECK(!hm.IsPaused());
    CHECK(hm.PauseReason().empty());
    CHECK(!hm.Resume());
    CHECK(hm.StatusOf("unknown") == Status::Healthy);
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}

#ifndef SHIVRAI_HON_SUBSCRIPTION_H
#define SHIVRAI_HON_SUBSCRIPTION_H
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
namespace shivrai::subscription {
using SubId = uint64_t;
struct Sub {
    SubId id{0};
    std::string subscriber;
    std::string provider;
    std::string plan;
    uint64_t amount{0};
    int64_t interval_sec{0};
    int64_t next_charge{0};
    int64_t created_at{0};
    uint64_t cycles_billed{0};
    uint64_t total_billed{0};
    bool active{true};
};
class Subscriptions {
public:
    Subscriptions();
    std::optional<SubId> Subscribe(const std::string& subscriber,
                                   const std::string& provider,
                                   const std::string& plan,
                                   uint64_t amount,
                                   int64_t interval_sec);
    bool Cancel(SubId id, const std::string& caller);
    bool Pause(SubId id, const std::string& caller);
    bool Resume(SubId id, const std::string& caller);
    std::optional<SubId> ChangePlan(SubId id, const std::string& caller,
                                    const std::string& new_plan, uint64_t new_amount);
    uint64_t Charge(SubId id);
    std::optional<Sub> Get(SubId id) const;
    std::vector<SubId> SubsOf(const std::string& subscriber) const;
    size_t Count() const;
private:
    mutable std::recursive_mutex cs_;
    std::map<SubId, Sub> subs_;
    std::map<std::string, std::vector<SubId>> by_subscriber_;
    SubId next_id_{1};
};
}
#endif

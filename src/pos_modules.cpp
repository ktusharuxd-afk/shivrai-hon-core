#include "shivrai/pos_modules.hpp"
#include <stdexcept>

namespace shivrai {

void register_pos(RpcServer& rpc, State& st, PoSConsensus& pos) {

  rpc.register_method("validator_register", [&](const json& q) {
    register_validator(st, q.value("address",""), q.value("pubkey",""),
                       q.value("stake",(uint64_t)0));
    auto v = get_validator(st, q.value("address",""));
    return json{{"address",v.address},{"stake",v.stake},{"active",v.active}};
  });

  rpc.register_method("validator_stake", [&](const json& q) {
    std::string a = q.value("address","");
    add_stake(st, a, q.value("amount",(uint64_t)0));
    auto v = get_validator(st, a);
    return json{{"address",v.address},{"stake",v.stake}};
  });

  rpc.register_method("validator_unstake", [&](const json& q) {
    std::string a = q.value("address","");
    if (!remove_stake(st, a, q.value("amount",(uint64_t)0)))
      throw std::runtime_error("unstake failed");
    auto v = get_validator(st, a);
    return json{{"address",v.address},{"stake",v.stake}};
  });

  rpc.register_method("validator_list", [&](const json&) {
    json arr = json::array();
    for (auto& v : list_validators(st))
      arr.push_back({{"address",v.address},{"pubkey",v.pubkey},
                     {"stake",v.stake},{"blocks_produced",v.blocks_produced},
                     {"active",v.active}});
    return json{{"count",arr.size()},{"validators",arr}};
  });

  rpc.register_method("validator_info", [&](const json& q) {
    auto v = get_validator(st, q.value("address",""));
    return json{{"address",v.address},{"pubkey",v.pubkey},{"stake",v.stake},
                {"blocks_produced",v.blocks_produced},{"active",v.active}};
  });

  rpc.register_method("validator_total_stake", [&](const json&) {
    return json{{"total_stake", total_stake(st)}};
  });

  rpc.register_method("pos_leader", [&](const json& q) {
    uint64_t h = q.value("height",(uint64_t)1);
    if (h == 0) h = 1;
    return json{{"height",h},{"leader",pos.select_leader(h)}};
  });
}
}

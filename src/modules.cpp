#include "shivrai/modules.hpp"
#include "shivrai/tx_validator.hpp"
#include <random>
namespace shivrai {
static std::string rand_id() {
  static std::mt19937_64 rng{std::random_device{}()};
  char b[17]; snprintf(b, sizeof(b), "%016llx", (unsigned long long)rng());
  return std::string(b);
}
void register_chain(RpcServer& rpc, Blockchain& ch, Mempool& p, State& st) {
  rpc.register_method("chain_info", [&](const json&) {
    return json{{"height",ch.height()},{"latest_hash",ch.latest().hash},{"mempool_size",p.size()}};
  });
  rpc.register_method("chain_getBlock", [&](const json& q) {
    return json(ch.get_block(q.value("height",0ULL)));
  });
  rpc.register_method("chain_latest", [&](const json&) { return json(ch.latest()); });
  rpc.register_method("account_balance", [&](const json& q) {
    std::string a=q.value("address","");
    return json{{"address",a},{"balance",st.balance(a)}};
  });
  rpc.register_method("tx_submit", [&](const json& q) {
    Transaction tx = q.get<Transaction>();
    if (tx.hash.empty()) tx.hash = ch.compute_tx_hash(tx);
    TxValidator v(st, p);
    std::string reason;
    if (!v.submit(tx, reason))
      throw std::runtime_error("rejected: " + reason);
    return json{{"hash",tx.hash},{"accepted",true}};
  });
  rpc.register_method("nonce_get", [&](const json& q) {
    std::string a=q.value("address","");
    return json{{"address",a},{"nonce",st.get_u64("nonce:"+a)}};
  });
}
void register_stablecoin(RpcServer& rpc, State& st) {
  rpc.register_method("stablecoin_mint", [&](const json& q) {
    std::string a=q.value("address",""); uint64_t n=q.value("amount",0ULL);
    uint64_t c=st.get_u64("stable:"+a); st.set_u64("stable:"+a,c+n);
    st.set_u64("stable:total_supply", st.get_u64("stable:total_supply")+n);
    return json{{"address",a},{"balance",c+n}};
  });
  rpc.register_method("stablecoin_balance", [&](const json& q) {
    std::string a=q.value("address","");
    return json{{"address",a},{"balance",st.get_u64("stable:"+a)}};
  });
  rpc.register_method("stablecoin_supply", [&](const json&) {
    return json{{"total_supply", st.get_u64("stable:total_supply")}};
  });
}
void register_lending(RpcServer& rpc, State& st) {
  rpc.register_method("lending_deposit", [&](const json& q) {
    std::string a=q.value("address",""); uint64_t n=q.value("amount",0ULL);
    uint64_t c=st.get_u64("lend:"+a); st.set_u64("lend:"+a,c+n);
    return json{{"address",a},{"deposited",c+n}};
  });
  rpc.register_method("lending_borrow", [&](const json& q) {
    std::string a=q.value("address",""); uint64_t n=q.value("amount",0ULL);
    uint64_t c=st.get_u64("borrow:"+a); st.set_u64("borrow:"+a,c+n);
    return json{{"address",a},{"borrowed",c+n}};
  });
  rpc.register_method("lending_position", [&](const json& q) {
    std::string a=q.value("address","");
    return json{{"address",a},{"deposits",st.get_u64("lend:"+a)},
                {"borrows",st.get_u64("borrow:"+a)}};
  });
}
void register_governance(RpcServer& rpc, State& st) {
  rpc.register_method("governance_propose", [&](const json& q) {
    std::string t=q.value("title","untitled");
    std::string id="prop:"+rand_id();
    st.set_str(id+":title",t); st.set_u64(id+":votes",0);
    return json{{"proposal_id",id},{"title",t}};
  });
  rpc.register_method("governance_vote", [&](const json& q) {
    std::string id=q.value("proposal_id","");
    uint64_t c=st.get_u64(id+":votes"); st.set_u64(id+":votes",c+1);
    return json{{"proposal_id",id},{"votes",c+1}};
  });
  rpc.register_method("governance_result", [&](const json& q) {
    std::string id=q.value("proposal_id","");
    return json{{"proposal_id",id},{"title",st.get_str(id+":title")},
                {"votes",st.get_u64(id+":votes")}};
  });
}
void register_token(RpcServer& rpc, State& st) {
  rpc.register_method("token_deploy", [&](const json& q) {
    std::string sym=q.value("symbol","TKN"); uint64_t sup=q.value("supply",0ULL);
    std::string id="tok:"+rand_id();
    st.set_str(id+":symbol",sym); st.set_u64(id+":supply",sup);
    return json{{"token_id",id},{"symbol",sym},{"supply",sup}};
  });
  rpc.register_method("token_info", [&](const json& q) {
    std::string id=q.value("token_id","");
    return json{{"token_id",id},{"symbol",st.get_str(id+":symbol")},
                {"supply",st.get_u64(id+":supply")}};
  });
}
}

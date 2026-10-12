#include "shivrai/modules_ext.hpp"
#include "shivrai/p2p.hpp"
#include <random>
#include <stdexcept>
#include <cstdio>
#include <tuple>
namespace shivrai {
static std::string rid() {
  static std::mt19937_64 rng{std::random_device{}()};
  char b[17]; snprintf(b,sizeof(b),"%016llx",(unsigned long long)rng());
  return std::string(b);
}
void register_nft(RpcServer& rpc, State& st) {
  rpc.register_method("nft_mint", [&](const json& q){
    std::string o=q.value("owner",""), u=q.value("uri","");
    std::string id="nft:"+rid();
    st.set_str(id+":owner",o); st.set_str(id+":uri",u);
    st.set_u64("nft:count", st.get_u64("nft:count")+1);
    return json{{"token_id",id},{"owner",o},{"uri",u}};
  });
  rpc.register_method("nft_ownerOf", [&](const json& q){
    std::string id=q.value("token_id","");
    return json{{"token_id",id},{"owner",st.get_str(id+":owner")}};
  });
  rpc.register_method("nft_transfer", [&](const json& q){
    std::string id=q.value("token_id",""), f=q.value("from",""), t=q.value("to","");
    if (st.get_str(id+":owner")!=f) throw std::runtime_error("not owner");
    st.set_str(id+":owner",t);
    return json{{"token_id",id},{"owner",t}};
  });
  rpc.register_method("nft_tokenURI", [&](const json& q){
    std::string id=q.value("token_id","");
    return json{{"token_id",id},{"uri",st.get_str(id+":uri")}};
  });
  rpc.register_method("nft_count", [&](const json&){
    return json{{"count", st.get_u64("nft:count")}};
  });
}
void register_amm(RpcServer& rpc, State& st) {
  rpc.register_method("amm_addLiquidity", [&](const json& q){
    std::string p=q.value("pool","default");
    uint64_t a=q.value("amount_a",0ULL), b=q.value("amount_b",0ULL);
    uint64_t ra=st.get_u64("amm:"+p+":a"), rb=st.get_u64("amm:"+p+":b");
    st.set_u64("amm:"+p+":a",ra+a); st.set_u64("amm:"+p+":b",rb+b);
    return json{{"pool",p},{"a",ra+a},{"b",rb+b}};
  });
  rpc.register_method("amm_reserves", [&](const json& q){
    std::string p=q.value("pool","default");
    uint64_t a=st.get_u64("amm:"+p+":a"), b=st.get_u64("amm:"+p+":b");
    return json{{"pool",p},{"a",a},{"b",b},{"k",a*b}};
  });
  rpc.register_method("amm_swap", [&](const json& q){
    std::string p=q.value("pool","default"), d=q.value("direction","a_to_b");
    uint64_t amt=q.value("amount_in",0ULL);
    uint64_t ra=st.get_u64("amm:"+p+":a"), rb=st.get_u64("amm:"+p+":b");
    if (ra==0||rb==0) throw std::runtime_error("empty pool");
    uint64_t out=0;
    if (d=="a_to_b") { out=rb-(ra*rb)/(ra+amt); st.set_u64("amm:"+p+":a",ra+amt); st.set_u64("amm:"+p+":b",rb-out); }
    else            { out=ra-(ra*rb)/(rb+amt); st.set_u64("amm:"+p+":b",rb+amt); st.set_u64("amm:"+p+":a",ra-out); }
    return json{{"pool",p},{"amount_in",amt},{"amount_out",out}};
  });
}
void register_staking(RpcServer& rpc, State& st, Blockchain& ch) {
  auto acc=[&](const std::string& a)->std::tuple<uint64_t,uint64_t,uint64_t>{
    uint64_t s=st.get_u64("stake:"+a+":amount");
    uint64_t l=st.get_u64("stake:"+a+":last_block");
    uint64_t c=ch.height();
    uint64_t r=st.get_u64("stake:"+a+":rewards");
    if (s>0 && c>l) r += s*(c-l)/1000;
    return {s,r,c};
  };
  rpc.register_method("staking_stake", [&, acc](const json& q){
    std::string a=q.value("address",""); uint64_t amt=q.value("amount",0ULL);
    auto [s,r,c]=acc(a);
    st.set_u64("stake:"+a+":amount",s+amt);
    st.set_u64("stake:"+a+":rewards",r);
    st.set_u64("stake:"+a+":last_block",c);
    return json{{"address",a},{"staked",s+amt},{"rewards",r}};
  });
  rpc.register_method("staking_unstake", [&, acc](const json& q){
    std::string a=q.value("address",""); uint64_t amt=q.value("amount",0ULL);
    auto [s,r,c]=acc(a);
    if (amt>s) throw std::runtime_error("insufficient stake");
    st.set_u64("stake:"+a+":amount",s-amt);
    st.set_u64("stake:"+a+":rewards",r);
    st.set_u64("stake:"+a+":last_block",c);
    return json{{"address",a},{"staked",s-amt},{"rewards",r}};
  });
  rpc.register_method("staking_info", [&, acc](const json& q){
    std::string a=q.value("address","");
    auto [s,r,c]=acc(a);
    return json{{"address",a},{"staked",s},{"rewards",r},{"last_block",c}};
  });
}
void register_multisig(RpcServer& rpc, State& st) {
  rpc.register_method("multisig_create", [&](const json& q){
    std::string id="ms:"+rid();
    auto owners=q.value("owners",std::vector<std::string>{});
    uint64_t th=q.value("threshold",(uint64_t)owners.size());
    st.set_str(id+":owners",json(owners).dump());
    st.set_u64(id+":threshold",th);
    st.set_u64(id+":signatures",0);
    st.set_u64(id+":executed",0);
    return json{{"wallet_id",id},{"owners",owners},{"threshold",th}};
  });
  rpc.register_method("multisig_sign", [&](const json& q){
    std::string id=q.value("wallet_id","");
    if (st.get_u64(id+":executed")) throw std::runtime_error("already executed");
    uint64_t cur=st.get_u64(id+":signatures")+1;
    st.set_u64(id+":signatures",cur);
    uint64_t th=st.get_u64(id+":threshold");
    if (cur>=th) st.set_u64(id+":executed",1);
    return json{{"wallet_id",id},{"signatures",cur},{"threshold",th},{"executed",cur>=th}};
  });
  rpc.register_method("multisig_info", [&](const json& q){
    std::string id=q.value("wallet_id","");
    auto owners=json::parse(st.get_str(id+":owners","[]"));
    return json{{"wallet_id",id},{"owners",owners},
                {"threshold",st.get_u64(id+":threshold")},
                {"signatures",st.get_u64(id+":signatures")},
                {"executed",st.get_u64(id+":executed")}};
  });
}
void register_p2p(RpcServer& rpc, P2P& p2p) {
  rpc.register_method("p2p_peers", [&](const json&){
    return json{{"peers", p2p.peers()}};
  });
  rpc.register_method("p2p_addPeer", [&](const json& q){
    std::string url=q.value("url","");
    if (url.empty()) throw std::runtime_error("missing url");
    p2p.add_peer(url);
    return json{{"added",url},{"peers",p2p.peers()}};
  });
}
void register_explorer(RpcServer& rpc, Blockchain& ch, State& st) {
  rpc.register_method("explorer_stats", [&](const json&){
    return json{{"height",ch.height()},{"latest_hash",ch.latest().hash},
                {"validator",ch.latest().proposer},
                {"nft_count",st.get_u64("nft:count")},
                {"stable_supply",st.get_u64("stable:total_supply")}};
  });
}
}

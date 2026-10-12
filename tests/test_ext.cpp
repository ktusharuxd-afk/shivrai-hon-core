#include "shivrai/storage.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/rpc.hpp"
#include "shivrai/modules_ext.hpp"
#include "shivrai/tx_validator.hpp"
#include "shivrai/wallet.hpp"
#include "shivrai/mempool.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#define CK(c) do { if(!(c)){std::cerr<<"FAIL: " #c " @" <<__LINE__<<"\n";std::exit(1);} else std::cout<<"  ok: " #c "\n"; } while(0)
using namespace shivrai;
int main() {
  std::remove("/tmp/ext.db");
  auto s=std::make_shared<Storage>("/tmp/ext.db"); s->init_schema();
  State st(s); Blockchain ch(s); RpcServer rpc;
  register_nft(rpc,st); register_amm(rpc,st);
  register_staking(rpc,st,ch); register_multisig(rpc,st);
  std::cout<<"[nft]\n";
  auto n=rpc.handle_request({{"jsonrpc","2.0"},{"id",1},{"method","nft_mint"},
    {"params",{{"owner","alice"},{"uri","ipfs://x"}}}});
  CK(n["result"]["owner"]=="alice");
  std::string tid=n["result"]["token_id"];
  auto o=rpc.handle_request({{"jsonrpc","2.0"},{"id",2},{"method","nft_ownerOf"},
    {"params",{{"token_id",tid}}}});
  CK(o["result"]["owner"]=="alice");
  auto t=rpc.handle_request({{"jsonrpc","2.0"},{"id",3},{"method","nft_transfer"},
    {"params",{{"token_id",tid},{"from","alice"},{"to","bob"}}}});
  CK(t["result"]["owner"]=="bob");
  std::cout<<"[amm]\n";
  rpc.handle_request({{"jsonrpc","2.0"},{"id",4},{"method","amm_addLiquidity"},
    {"params",{{"pool","p1"},{"amount_a",1000},{"amount_b",1000}}}});
  auto r=rpc.handle_request({{"jsonrpc","2.0"},{"id",5},{"method","amm_reserves"},
    {"params",{{"pool","p1"}}}});
  CK(r["result"]["a"]==1000);
  auto sw=rpc.handle_request({{"jsonrpc","2.0"},{"id",6},{"method","amm_swap"},
    {"params",{{"pool","p1"},{"amount_in",100},{"direction","a_to_b"}}}});
  CK(sw["result"]["amount_out"]>0);
  std::cout<<"[staking]\n";
  rpc.handle_request({{"jsonrpc","2.0"},{"id",7},{"method","staking_stake"},
    {"params",{{"address","alice"},{"amount",500}}}});
  auto si=rpc.handle_request({{"jsonrpc","2.0"},{"id",8},{"method","staking_info"},
    {"params",{{"address","alice"}}}});
  CK(si["result"]["staked"]==500);
  std::cout<<"[multisig]\n";
  auto ms=rpc.handle_request({{"jsonrpc","2.0"},{"id",9},{"method","multisig_create"},
    {"params",{{"owners",{"a","b","c"}},{"threshold",2}}}});
  std::string wid=ms["result"]["wallet_id"];
  auto s1=rpc.handle_request({{"jsonrpc","2.0"},{"id",10},{"method","multisig_sign"},
    {"params",{{"wallet_id",wid}}}});
  CK(s1["result"]["signatures"]==1);
  CK(s1["result"]["executed"]==false);
  auto s2=rpc.handle_request({{"jsonrpc","2.0"},{"id",11},{"method","multisig_sign"},
    {"params",{{"wallet_id",wid}}}});
  CK(s2["result"]["executed"]==true);
  std::cout<<"[explorer]\n";
  { register_explorer(rpc,ch,st);
    auto e=rpc.handle_request({{"jsonrpc","2.0"},{"id",99},{"method","explorer_stats"},{"params",{}}});
    CK(e["result"].contains("height"));
    CK(e["result"]["nft_count"].get<uint64_t>()>=1); }
  std::cout<<"[tx_validator]\n";
  { Mempool mp; Wallet w;
    st.set_balance(w.address(), 1000);
    st.set_u64("nonce:"+w.address(), 0);
    auto tx = w.create_tx("0xdead", 10, 0, "transfer");
    TxValidator v(st, mp);
    std::string r;
    CK(v.submit(tx, r));       // first ok
    CK(!v.submit(tx, r));      // replay rejected
    auto bad = w.create_tx("0xdead", 10, 5, "transfer");  // wrong nonce
    CK(!v.submit(bad, r));
    auto over = w.create_tx("0xdead", 999999, 1, "transfer"); // too much
    CK(!v.submit(over, r)); }
  std::cout<<"\nALL EXT TESTS PASSED\n"; return 0;
}

#include "shivrai/crypto.hpp"
#include "shivrai/storage.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/wallet.hpp"
#include "shivrai/mempool.hpp"
#include "shivrai/rpc.hpp"
#include "shivrai/modules.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#define CHECK(c) do { if(!(c)){std::cerr<<"FAIL: " #c " @" <<__LINE__<<"\n";std::exit(1);} else std::cout<<"  ok: " #c "\n"; } while(0)
using namespace shivrai;
int main() {
  std::cout<<"[crypto]\n";
  { auto kp=crypto::generate_keypair(); CHECK(kp.private_key_hex.size()==64);
    auto sig=crypto::sign(kp.private_key_hex,"hello");
    CHECK(crypto::verify(kp.public_key_hex,"hello",sig));
    CHECK(!crypto::verify(kp.public_key_hex,"world",sig)); }
  std::cout<<"[storage]\n";
  { std::remove("/tmp/st.db"); Storage s("/tmp/st.db"); s.init_schema();
    s.put("t","k","v"); auto v=s.get("t","k"); CHECK(v && *v=="v"); }
  std::cout<<"[state]\n";
  { std::remove("/tmp/s.db"); auto s=std::make_shared<Storage>("/tmp/s.db");
    s->init_schema(); State st(s); st.mint("a",100); CHECK(st.balance("a")==100); }
  std::cout<<"[chain]\n";
  { std::remove("/tmp/c.db"); auto s=std::make_shared<Storage>("/tmp/c.db");
    s->init_schema(); Blockchain c(s); CHECK(c.height()==0);
    c.add_block({}, "v", "r"); CHECK(c.height()==1); }
  std::cout<<"[wallet]\n";
  { Wallet w; auto tx=w.create_tx("b",42,1,"transfer");
    CHECK(tx.from==w.address()); CHECK(!tx.hash.empty()); }
  std::cout<<"[mempool]\n";
  { Mempool m; Wallet w; m.add(w.create_tx("x",1,1)); CHECK(m.size()==1); }
  std::cout<<"[rpc]\n";
  { RpcServer r; r.register_method("echo",[](const json&p){return json{{"echo",p.value("msg","")}};});
    auto resp=r.handle_request({{"jsonrpc","2.0"},{"id",1},{"method","echo"},{"params",{{"msg","hi"}}}});
    CHECK(resp["result"]["echo"]=="hi"); }
  std::cout<<"[modules]\n";
  { std::remove("/tmp/m.db"); auto s=std::make_shared<Storage>("/tmp/m.db");
    s->init_schema(); State st(s); RpcServer r; register_stablecoin(r,st);
    auto x=r.handle_request({{"jsonrpc","2.0"},{"id",1},{"method","stablecoin_mint"},
      {"params",{{"address","a"},{"amount",100}}}});
    CHECK(x["result"]["balance"]==100); }
  std::cout<<"\nALL TESTS PASSED\n";
  return 0;
}

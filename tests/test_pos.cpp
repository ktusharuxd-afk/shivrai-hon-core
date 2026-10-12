#include "shivrai/storage.hpp"
#include "shivrai/state.hpp"
#include "shivrai/blockchain.hpp"
#include "shivrai/wallet.hpp"
#include "shivrai/mempool.hpp"
#include "shivrai/pos.hpp"
#include "shivrai/pos_modules.hpp"
#include "shivrai/rpc.hpp"
#include <iostream>
#include <cstdio>
#include <cstdlib>

#define CK(c) do { if(!(c)){std::cerr<<"FAIL: " #c " @" <<__LINE__<<"\n";std::exit(1);} \
  else std::cout<<"  ok: " #c "\n"; } while(0)
using namespace shivrai;

int main() {
  std::remove("/tmp/pos_test.db");
  auto s = std::make_shared<Storage>("/tmp/pos_test.db");
  s->init_schema();
  State st(s);
  Blockchain ch(s);
  Mempool mp;
  Wallet w;

  std::cout << "[registry]\n";
  st.mint("alice", 1000);
  st.mint("bob", 1000);
  register_validator(st, "alice", "pubAlice", 500);
  register_validator(st, "bob",   "pubBob",   300);
  CK(st.balance("alice") == 500);
  CK(total_stake(st) == 800);
  CK(list_validators(st).size() == 2);

  std::cout << "[stake ops]\n";
  add_stake(st, "bob", 100);
  CK(get_validator(st, "bob").stake == 400);
  CK(remove_stake(st, "bob", 200));
  CK(get_validator(st, "bob").stake == 200);

  std::cout << "[slashing]\n";
  slash_validator(st, "bob", 200);
  CK(get_validator(st, "bob").stake == 0);
  CK(!get_validator(st, "bob").active);

  std::cout << "[leader]\n";
  PoSConsensus pos(ch, mp, st, w, 3000);
  CK(pos.select_leader(1) == "alice");

  std::cout << "[rpc]\n";
  RpcServer rpc;
  register_pos(rpc, st, pos);
  auto r1 = rpc.handle_request({{"jsonrpc","2.0"},{"id",1},
    {"method","validator_list"},{"params",{}}});
  CK(r1["result"]["count"] == 2);
  auto r2 = rpc.handle_request({{"jsonrpc","2.0"},{"id",2},
    {"method","validator_info"},{"params",{{"address","alice"}}}});
  CK(r2["result"]["stake"] == 500);
  auto r3 = rpc.handle_request({{"jsonrpc","2.0"},{"id",3},
    {"method","validator_total_stake"},{"params",{}}});
  CK(r3["result"]["total_stake"] == 500);

  std::cout << "\nALL POS TESTS PASSED\n";
  return 0;
}

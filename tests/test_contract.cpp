#include "shivrai/storage.hpp"
#include "shivrai/state.hpp"
#include "shivrai/contract.hpp"
#include "shivrai/contract_modules.hpp"
#include "shivrai/rpc.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

#define CK(c) do { if(!(c)){std::cerr<<"FAIL: " #c " @" <<__LINE__<<"\n";std::exit(1);} \
  else std::cout<<"  ok: " #c "\n"; } while(0)
using namespace shivrai;

static std::vector<uint8_t> read_file(const std::string& p) {
  // Try several locations: cwd, build/, build/../
  const char* prefixes[] = {"", "../", "../../"};
  for (auto pre : prefixes) {
    std::ifstream f(std::string(pre) + p, std::ios::binary);
    if (f.good()) {
      return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                   std::istreambuf_iterator<char>());
    }
  }
  return {};
}

int main() {
  std::remove("/tmp/contract_test.db");
  auto s = std::make_shared<Storage>("/tmp/contract_test.db");
  s->init_schema();
  State st(s);
  ContractEngine e(st);

  auto wasm = read_file("examples/contracts/hello.wasm");
  if (wasm.empty()) { std::cerr << "run from repo root\n"; return 1; }

  std::cout << "[deploy]\n";
  std::string addr = e.deploy(wasm, "alice");
  CK(!addr.empty());
  CK(addr.substr(0,3) == "0xc");
  CK(e.exists(addr));
  CK(e.get(addr).code.size() == wasm.size());

  std::cout << "[call]\n";
  auto r = e.call(addr, "bob", "hello");
  CK(r.success);
  CK(r.return_value == 42);
  std::cout << "  gas_used=" << r.gas_used << "\n";

  std::cout << "[list]\n";
  auto lst = e.list();
  CK(lst.size() == 1);
  CK(lst[0] == addr);

  std::cout << "[rpc]\n";
  RpcServer rpc;
  register_contract(rpc, st);
  auto info = rpc.handle_request({{"jsonrpc","2.0"},{"id",1},
    {"method","contract_info"},{"params",{{"address",addr}}}});
  CK(info["result"]["owner"] == "alice");
  auto lst2 = rpc.handle_request({{"jsonrpc","2.0"},{"id",2},
    {"method","contract_list"},{"params",{}}});
  CK(lst2["result"]["contracts"].size() == 1);

  std::cout << "[bad wasm rejected]\n";
  bool threw = false;
  try { e.deploy({0x00,0x01,0x02}, "x"); } catch (...) { threw = true; }
  CK(threw);

  std::cout << "\nALL CONTRACT TESTS PASSED\n";
  return 0;
}

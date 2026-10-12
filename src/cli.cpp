#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include "shivrai/crypto.hpp"
using json = nlohmann::json;
using namespace shivrai;
static void usage() {
  std::cout << "shivrai-cli <cmd> [args] [--url URL]\n"
    "  keygen | info | balance <addr> | stable-mint <addr> <amt>\n"
    "  nft-mint <owner> <uri> | swap <pool> <amt> <a_to_b|b_to_a>\n"
    "  stake <addr> <amt> | call <method> <json>\n";
}
static json rpc_call(const std::string& url, const std::string& m, const json& p) {
  httplib::Client cli(url);
  json req={{"jsonrpc","2.0"},{"id",1},{"method",m},{"params",p}};
  auto res=cli.Post("/rpc", req.dump(), "application/json");
  if (!res) throw std::runtime_error("connection failed");
  auto r=json::parse(res->body);
  if (r.contains("error")) throw std::runtime_error(r["error"].dump());
  return r["result"];
}
int main(int argc, char** argv) {
  std::string url="http://127.0.0.1:8545";
  std::vector<std::string> a;
  for (int i=1;i<argc;++i) {
    std::string s=argv[i];
    if (s=="--url" && i+1<argc) { url=argv[++i]; continue; }
    a.push_back(s);
  }
  if (a.empty()) { usage(); return 1; }
  try {
    if (a[0]=="keygen") {
      auto kp=crypto::generate_keypair();
      std::cout<<"address:     "<<kp.address<<"\n"
               <<"private_key: "<<kp.private_key_hex<<"\n"
               <<"public_key:  "<<kp.public_key_hex<<"\n";
    } else if (a[0]=="info") {
      std::cout << rpc_call(url,"chain_info",json::object()).dump(2) << "\n";
    } else if (a[0]=="balance" && a.size()>=2) {
      std::cout << rpc_call(url,"account_balance",{{"address",a[1]}}).dump(2) << "\n";
    } else if (a[0]=="stable-mint" && a.size()>=3) {
      std::cout << rpc_call(url,"stablecoin_mint",
        {{"address",a[1]},{"amount",std::stoull(a[2])}}).dump(2) << "\n";
    } else if (a[0]=="nft-mint" && a.size()>=3) {
      std::cout << rpc_call(url,"nft_mint",
        {{"owner",a[1]},{"uri",a[2]}}).dump(2) << "\n";
    } else if (a[0]=="swap" && a.size()>=4) {
      std::cout << rpc_call(url,"amm_swap",
        {{"pool",a[1]},{"amount_in",std::stoull(a[2])},{"direction",a[3]}}).dump(2) << "\n";
    } else if (a[0]=="stake" && a.size()>=3) {
      std::cout << rpc_call(url,"staking_stake",
        {{"address",a[1]},{"amount",std::stoull(a[2])}}).dump(2) << "\n";
    } else if (a[0]=="call" && a.size()>=2) {
      json p = a.size()>=3 ? json::parse(a[2]) : json::object();
      std::cout << rpc_call(url,a[1],p).dump(2) << "\n";
    } else { usage(); return 1; }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n"; return 1;
  }
  return 0;
}

#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
namespace shivrai {
using json = nlohmann::json;
struct Transaction {
  std::string hash, from, to;
  std::string pubkey;
  uint64_t value=0, nonce=0, timestamp=0;
  std::string method;
  json params = json::object();
  std::string signature;
};
struct Block {
  uint64_t height=0;
  std::string parent_hash, hash;
  uint64_t timestamp=0;
  std::string proposer;
  std::vector<Transaction> transactions;
  std::string state_root;
};
inline void to_json(json& j, const Transaction& t) {
  j = json{{"hash",t.hash},{"from",t.from},{"to",t.to},{"pubkey",t.pubkey},{"value",t.value},
    {"nonce",t.nonce},{"timestamp",t.timestamp},{"method",t.method},
    {"params",t.params},{"signature",t.signature}};
}
inline void from_json(const json& j, Transaction& t) {
  j.at("hash").get_to(t.hash); j.at("from").get_to(t.from); j.at("to").get_to(t.to);
  t.pubkey=j.value("pubkey","");
  t.value=j.value("value",0ULL); t.nonce=j.value("nonce",0ULL);
  t.timestamp=j.value("timestamp",0ULL); t.method=j.value("method","");
  t.params=j.value("params",json::object()); t.signature=j.value("signature","");
}
inline void to_json(json& j, const Block& b) {
  j = json{{"height",b.height},{"parent_hash",b.parent_hash},{"hash",b.hash},
    {"timestamp",b.timestamp},{"proposer",b.proposer},
    {"transactions",b.transactions},{"state_root",b.state_root}};
}
inline void from_json(const json& j, Block& b) {
  b.height=j.value("height",0ULL); b.parent_hash=j.value("parent_hash","");
  b.hash=j.value("hash",""); b.timestamp=j.value("timestamp",0ULL);
  b.proposer=j.value("proposer","");
  b.transactions=j.value("transactions",std::vector<Transaction>{});
  b.state_root=j.value("state_root","");
}
}

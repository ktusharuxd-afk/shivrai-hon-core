#include "shivrai/contract_modules.hpp"
#include <stdexcept>

namespace shivrai {

static std::vector<uint8_t> decode_hex(const std::string& s) {
  std::vector<uint8_t> o;
  if (s.size() % 2) throw std::runtime_error("hex length must be even");
  for (size_t i = 0; i + 1 < s.size(); i += 2)
    o.push_back((uint8_t)std::stoi(s.substr(i,2), nullptr, 16));
  return o;
}

void register_contract(RpcServer& rpc, State& st) {
  rpc.register_method("contract_deploy", [&](const json& q) {
    std::string hex = q.value("wasm_hex", "");
    std::string owner = q.value("owner", "");
    if (hex.empty()) throw std::runtime_error("wasm_hex required");
    ContractEngine e(st);
    auto bytes = decode_hex(hex);
    std::string addr = e.deploy(bytes, owner);
    return json{{"address", addr}, {"size", bytes.size()}};
  });

  rpc.register_method("contract_call", [&](const json& q) {
    std::string addr   = q.value("address", "");
    std::string caller = q.value("caller", "");
    std::string arg    = q.value("arg", "");
    ContractEngine e(st);
    auto res = e.call(addr, caller, arg);
    return json{{"success", res.success}, {"error", res.error},
                {"return_value", res.return_value}, {"gas_used", res.gas_used},
                {"events", res.events}};
  });

  rpc.register_method("contract_info", [&](const json& q) {
    std::string addr = q.value("address", "");
    ContractEngine e(st);
    auto c = e.get(addr);
    if (c.code.empty()) throw std::runtime_error("not found");
    return json{{"address", c.address}, {"owner", c.owner},
                {"deploy_block", c.deploy_block}, {"call_count", c.call_count},
                {"size", c.code.size()}};
  });

  rpc.register_method("contract_list", [&](const json&) {
    ContractEngine e(st);
    return json{{"contracts", e.list()}};
  });

  rpc.register_method("contract_code", [&](const json& q) {
    std::string addr = q.value("address", "");
    ContractEngine e(st);
    auto c = e.get(addr);
    if (c.code.empty()) throw std::runtime_error("not found");
    static const char* hx = "0123456789abcdef";
    std::string hex;
    for (auto b : c.code) { hex.push_back(hx[b>>4]); hex.push_back(hx[b&0xF]); }
    return json{{"address", addr}, {"wasm_hex", hex}};
  });
}

}

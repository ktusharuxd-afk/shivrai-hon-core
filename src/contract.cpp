#include "shivrai/contract.hpp"
#include "shivrai/crypto.hpp"
#include <cstring>
#include <ctime>
#include <algorithm>
#include <stdexcept>

extern "C" {
#include "wasm3.h"
#include "m3_env.h"
}

namespace shivrai {

static std::string bytes_to_hex(const std::vector<uint8_t>& v) {
  static const char* hx = "0123456789abcdef";
  std::string o; o.reserve(v.size() * 2);
  for (auto b : v) { o.push_back(hx[b >> 4]); o.push_back(hx[b & 0xF]); }
  return o;
}
static std::vector<uint8_t> hex_to_bytes(const std::string& s) {
  std::vector<uint8_t> o;
  for (size_t i = 0; i + 1 < s.size(); i += 2)
    o.push_back((uint8_t)std::stoi(s.substr(i,2), nullptr, 16));
  return o;
}

// ---- per-call context ----
struct CallCtx {
  State*  state = nullptr;
  std::string addr;
  std::string caller;
  uint64_t gas_used = 0;
  uint64_t gas_limit = 1'000'000;
  std::vector<std::string>* events = nullptr;
};
static CallCtx* g_ctx = nullptr;

// ---- host functions ----
// wasm3 raw signature "i(iiii)": ret i32, args (i32,i32,i32,i32)
static m3ApiRawFunction(h_storage_set) {
  m3ApiReturnType(int32_t)
  m3ApiGetArgMem(const uint8_t*, kp)
  m3ApiGetArg(uint32_t, klen)
  m3ApiGetArgMem(const uint8_t*, vp)
  m3ApiGetArg(uint32_t, vlen)

  g_ctx->gas_used += 100;
  if (g_ctx->gas_used > g_ctx->gas_limit) { *raw_return = -1; m3ApiSuccess(); }

  std::string key((const char*)kp, klen);
  std::string val((const char*)vp, vlen);
  g_ctx->state->set_str("contract:" + g_ctx->addr + ":s:" + key, val);
  *raw_return = 0;
  m3ApiSuccess();
}

static m3ApiRawFunction(h_storage_get) {
  m3ApiReturnType(int32_t)
  m3ApiGetArgMem(const uint8_t*, kp)
  m3ApiGetArg(uint32_t, klen)
  m3ApiGetArgMem(uint8_t*, op)
  m3ApiGetArg(uint32_t, olen)

  g_ctx->gas_used += 50;
  std::string key((const char*)kp, klen);
  std::string val = g_ctx->state->get_str("contract:" + g_ctx->addr + ":s:" + key, "");
  if (val.empty() || val.size() > olen) { *raw_return = -1; m3ApiSuccess(); }
  std::memcpy(op, val.data(), val.size());
  *raw_return = (int32_t)val.size();
  m3ApiSuccess();
}

// signature "v(ii)": void, (i32,i32)
static m3ApiRawFunction(h_emit_event) {
  m3ApiGetArgMem(const uint8_t*, p)
  m3ApiGetArg(uint32_t, l)
  g_ctx->gas_used += 20;
  if (g_ctx->events) g_ctx->events->push_back(std::string((const char*)p, l));
  m3ApiSuccess();
}

// signature "v(i)"
static m3ApiRawFunction(h_gas_charge) {
  m3ApiGetArg(uint32_t, amt)
  g_ctx->gas_used += amt;
  m3ApiSuccess();
}

// signature "i(ii)"
static m3ApiRawFunction(h_get_caller) {
  m3ApiReturnType(int32_t)
  m3ApiGetArgMem(uint8_t*, op)
  m3ApiGetArg(uint32_t, olen)
  const std::string& c = g_ctx->caller;
  uint32_t n = (uint32_t)std::min<size_t>(c.size(), olen);
  std::memcpy(op, c.data(), n);
  *raw_return = (int32_t)n;
  m3ApiSuccess();
}

// ---- ContractEngine ----
ContractEngine::ContractEngine(State& s) : state_(s) {}

bool ContractEngine::exists(const std::string& addr) const {
  return !state_.get_str("contract:" + addr + ":code", "").empty();
}

std::string ContractEngine::deploy(const std::vector<uint8_t>& wasm,
                                    const std::string& owner) {
  if (wasm.empty()) throw std::runtime_error("empty wasm");
  if (wasm.size() < 4 || wasm[0]!=0x00 || wasm[1]!=0x61 ||
      wasm[2]!=0x73 || wasm[3]!=0x6d)
    throw std::runtime_error("not a valid wasm module (bad magic)");

  std::string seed = bytes_to_hex(wasm) + owner + std::to_string(std::time(nullptr));
  std::string addr = "0xc" + crypto::sha256_hex(seed).substr(0, 39);

  state_.set_str("contract:" + addr + ":code", bytes_to_hex(wasm));
  state_.set_str("contract:" + addr + ":owner", owner);
  state_.set_u64("contract:" + addr + ":block", state_.get_u64("height"));
  state_.set_u64("contract:" + addr + ":calls", 0);

  std::string list = state_.get_str("contract:list", "");
  if (!list.empty()) list += ",";
  list += addr;
  state_.set_str("contract:list", list);
  return addr;
}

ContractInfo ContractEngine::get(const std::string& addr) const {
  ContractInfo c;
  c.address = addr;
  c.owner = state_.get_str("contract:" + addr + ":owner", "");
  std::string code_hex = state_.get_str("contract:" + addr + ":code", "");
  if (!code_hex.empty()) c.code = hex_to_bytes(code_hex);
  c.deploy_block = state_.get_u64("contract:" + addr + ":block", 0);
  c.call_count   = state_.get_u64("contract:" + addr + ":calls", 0);
  return c;
}

std::vector<std::string> ContractEngine::list() const {
  std::vector<std::string> out;
  std::string csv = state_.get_str("contract:list", "");
  size_t p = 0;
  while (p < csv.size()) {
    size_t c = csv.find(',', p);
    if (c == std::string::npos) c = csv.size();
    if (c > p) out.push_back(csv.substr(p, c - p));
    p = c + 1;
  }
  return out;
}

ContractCallResult ContractEngine::call(const std::string& addr,
                                         const std::string& caller,
                                         const std::string& arg) {
  ContractCallResult r;
  ContractInfo ci = get(addr);
  if (ci.code.empty()) { r.error = "contract not found"; return r; }

  CallCtx ctx;
  ctx.state = &state_;
  ctx.addr = addr;
  ctx.caller = caller;
  ctx.events = &r.events;
  g_ctx = &ctx;

  IM3Environment env = m3_NewEnvironment();
  IM3Runtime     rt  = m3_NewRuntime(env, 64*1024, nullptr);
  IM3Module      mod = nullptr;

  auto cleanup = [&]() {
    g_ctx = nullptr;
    if (rt)  m3_FreeRuntime(rt);
    if (env) m3_FreeEnvironment(env);
  };

  if (!env || !rt) { r.error = "runtime init failed"; cleanup(); return r; }

  M3Result res = m3_ParseModule(env, &mod, ci.code.data(), (uint32_t)ci.code.size());
  if (res) { r.error = std::string("parse: ") + res; cleanup(); return r; }

  res = m3_LoadModule(rt, mod);
  if (res) { r.error = std::string("load: ") + res; cleanup(); return r; }

  // link host functions (signatures must match)
  m3_LinkRawFunction(mod, "env", "storage_set", "i(iiii)", &h_storage_set);
  m3_LinkRawFunction(mod, "env", "storage_get", "i(iiii)", &h_storage_get);
  m3_LinkRawFunction(mod, "env", "emit_event",  "v(ii)",   &h_emit_event);
  m3_LinkRawFunction(mod, "env", "gas_charge",  "v(i)",    &h_gas_charge);
  m3_LinkRawFunction(mod, "env", "get_caller",  "i(ii)",   &h_get_caller);

  IM3Function fn = nullptr;
  res = m3_FindFunction(&fn, rt, "call");
  if (res) { r.error = std::string("no 'call' export: ") + res; cleanup(); return r; }

  // write arg at offset 0 of linear memory
  uint8_t* mem = m3_GetMemory(mod, nullptr, 0);
  if (!mem) { r.error = "no linear memory"; cleanup(); return r; }
  uint32_t argp = 0;
  uint32_t argl = (uint32_t)arg.size();
  if (argl > 0) std::memcpy(mem, arg.data(), argl);

  const void* args[2] = { &argp, &argl };
  res = m3_Call(fn, 2, args);
  if (res) { r.error = std::string("call: ") + res; cleanup(); return r; }

  int32_t ret = 0;
  const void* ret_ptr = (const void*)&ret;
  res = m3_GetResults(fn, 1, &ret_ptr);
  if (res) { r.error = std::string("result: ") + res; cleanup(); return r; }

  r.return_value = ret;
  r.gas_used = ctx.gas_used;
  r.success = true;

  state_.set_u64("contract:" + addr + ":calls", ci.call_count + 1);
  cleanup();
  return r;
}

}

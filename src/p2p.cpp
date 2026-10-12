#include "shivrai/p2p.hpp"
#include "shivrai/blockchain.hpp"
#include "httplib.h"
#include <thread>
#include <chrono>
#include <cstdio>
#include <stdexcept>

namespace shivrai {

static std::string rpc_post(const std::string& host, int port,
                            const std::string& method, const json& params) {
  httplib::Client cli(host, port);
  cli.set_connection_timeout(2, 0);
  json req = {{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}};
  auto res = cli.Post("/rpc", req.dump(), "application/json");
  if (!res || res->status != 200) {
    throw std::runtime_error("POST failed: " + std::to_string(res ? res->status : 0));
  }
  auto j = json::parse(res->body);
  if (j.contains("error")) throw std::runtime_error(j["error"].dump());
  return j["result"].dump();
}

static void parse_url(const std::string& url, std::string& host, int& port) {
  std::string u = url;
  if (u.rfind("http://", 0) == 0) u = u.substr(7);
  auto pos = u.find(':');
  host = u.substr(0, pos);
  port = (pos == std::string::npos) ? 80 : std::stoi(u.substr(pos + 1));
}

P2P::P2P(Blockchain& c) : chain_(c) {}

void P2P::add_peer(const std::string& url) {
  std::lock_guard<std::mutex> l(mu_);
  for (auto& p : peers_) if (p == url) return;
  peers_.push_back(url);
  fprintf(stderr, "[p2p] peer added: %s (total %zu)\n", url.c_str(), peers_.size());
}

std::vector<std::string> P2P::peers() {
  std::lock_guard<std::mutex> l(mu_);
  return peers_;
}

void P2P::broadcast_block(const Block& b) {
  std::vector<std::string> snap;
  { std::lock_guard<std::mutex> l(mu_); snap = peers_; }
  if (snap.empty()) return;
  std::string body = json(b).dump();
  std::thread([snap, body]() {
    for (auto& url : snap) {
      try {
        std::string host; int port;
        parse_url(url, host, port);
        httplib::Client cli(host, port);
        cli.set_connection_timeout(2, 0);
        auto res = cli.Post("/p2p/block", body, "application/json");
        fprintf(stderr, "[p2p] -> %s : %s\n", url.c_str(),
                res ? std::to_string(res->status).c_str() : "fail");
      } catch (const std::exception& e) {
        fprintf(stderr, "[p2p] -> %s err: %s\n", url.c_str(), e.what());
      }
    }
  }).detach();
}

bool P2P::receive_block(const Block& b, std::string& reason) {
  auto tip = chain_.latest();
  if (b.height != tip.height + 1) {
    reason = "height mismatch (want " + std::to_string(tip.height + 1) +
             ", got " + std::to_string(b.height) + ")";
    return false;
  }
  if (b.parent_hash != tip.hash) { reason = "parent mismatch"; return false; }
  chain_.import_block(b);
  fprintf(stderr, "[p2p] accepted block %llu\n", (unsigned long long)b.height);
  return true;
}

bool P2P::sync_from_peer(const std::string& url) {
  for (int attempt = 0; attempt < 20; ++attempt) {
    try {
      std::string host; int port;
      parse_url(url, host, port);

      // 1) Get peer's tip block (via POST /rpc — reliable)
      auto peer_tip = json::parse(
        rpc_post(host, port, "chain_latest", json::object())).get<Block>();
      auto my_tip = chain_.latest();

      if (peer_tip.height <= my_tip.height) {
        fprintf(stderr, "[p2p] sync: peer=%llu me=%llu (in sync)\n",
                (unsigned long long)peer_tip.height,
                (unsigned long long)my_tip.height);
        return true;
      }

      fprintf(stderr, "[p2p] pulling %llu blocks from %s...\n",
              (unsigned long long)(peer_tip.height - my_tip.height), url.c_str());

      // 2) Pull each block, one by one
      for (uint64_t h = my_tip.height + 1; h <= peer_tip.height; ++h) {
        auto b = json::parse(
          rpc_post(host, port, "chain_getBlock",
                   json{{"height", h}})).get<Block>();
        chain_.import_block(b);
      }
      fprintf(stderr, "[p2p] sync complete, now at height %llu\n",
              (unsigned long long)chain_.height());
      return true;
    } catch (const std::exception& e) {
      fprintf(stderr, "[p2p] sync attempt %d/20: %s\n", attempt + 1, e.what());
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
  }
  return false;
}

}

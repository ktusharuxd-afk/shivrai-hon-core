#pragma once
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
namespace shivrai {
using json = nlohmann::json;
class RpcServer {
public:
  using Handler = std::function<json(const json&)>;
  RpcServer(); ~RpcServer();
  void register_method(const std::string&, Handler);
  json handle_request(const json&);
  void start(const std::string&, int);
  void stop();
  using P2PReceiver = std::function<bool(const nlohmann::json&, std::string&)>;
  void set_p2p_receiver(P2PReceiver r);
private:
  std::map<std::string, Handler> methods_;
  std::mutex mu_;
  struct Impl; Impl* impl_;
};
}

#include "shivrai/rpc.hpp"
#include "httplib.h"
#include <functional>
#include <sstream>
#include <thread>
namespace shivrai {
struct RpcServer::Impl {
  std::function<bool(const json&, std::string&)> p2p_receiver;
  httplib::Server srv;
  std::thread thr;
};
RpcServer::RpcServer() : impl_(new Impl()) {}
RpcServer::~RpcServer() { stop(); delete impl_; }
void RpcServer::register_method(const std::string& n, Handler h) {
  std::lock_guard<std::mutex> l(mu_); methods_[n] = std::move(h);
}
json RpcServer::handle_request(const json& req) {
  auto respond = [](const json& id, const json& r, const json& e) {
    json o = {{"jsonrpc","2.0"},{"id",id}};
    if (!e.is_null()) o["error"]=e; else o["result"]=r;
    return o;
  };
  if (req.is_array()) {
    json out = json::array();
    for (auto& r : req) out.push_back(handle_request(r));
    return out;
  }
  json id = req.value("id", json(nullptr));
  if (!req.contains("method"))
    return respond(id, nullptr, {{"code",-32600},{"message","invalid request"}});
  std::string method = req["method"];
  json params = req.value("params", json::object());
  Handler h;
  {
    std::lock_guard<std::mutex> l(mu_);
    auto it = methods_.find(method);
    if (it == methods_.end())
      return respond(id, nullptr, {{"code",-32601},{"message","method not found"}});
    h = it->second;
  }
  try { return respond(id, h(params), nullptr); }
  catch (const std::exception& e) {
    return respond(id, nullptr, {{"code",-32000},{"message",e.what()}});
  }
}
void RpcServer::start(const std::string& host, int port) {
  impl_->srv.set_mount_point("/", "./static");
  impl_->srv.Get("/health", [](const httplib::Request&, httplib::Response& res) {
    res.set_content(R"({"status":"ok"})", "application/json");
  });
  impl_->srv.Post("/rpc", [this](const httplib::Request& req, httplib::Response& res) {
    try {
      auto body = json::parse(req.body);
      res.set_content(handle_request(body).dump(), "application/json");
    } catch (const std::exception& e) {
      res.status = 400;
      res.set_content(json{{"error",e.what()}}.dump(), "application/json");
    }
  });
  impl_->srv.Post("/p2p/block", [this](const httplib::Request& req, httplib::Response& res){
    if (!impl_->p2p_receiver) { res.status=503; res.set_content("{\"error\":\"off\"}","application/json"); return; }
    try { json b=json::parse(req.body); std::string r;
      if (impl_->p2p_receiver(b,r)) res.set_content("{\"ok\":true}","application/json");
      else { res.status=400; res.set_content(json{{"error",r}}.dump(),"application/json"); }
    } catch(const std::exception&e){ res.status=400; res.set_content(json{{"error",e.what()}}.dump(),"application/json"); }
  });
  impl_->srv.Get(R"(/api/v1/rpc/([A-Za-z0-9_]+))", [this](const httplib::Request& req, httplib::Response& res){
    std::string method = req.matches[1];
    try {
      json params = req.has_param("params") ? json::parse(req.get_param_value("params")) : json::object();
      json r = handle_request({{"jsonrpc","2.0"},{"id",1},{"method",method},{"params",params}});
      res.set_content(r.dump(), "application/json");
    } catch (const std::exception& e) {
      res.status = 400; res.set_content(json{{"error",e.what()}}.dump(), "application/json");
    }
  });
  impl_->thr = std::thread([this,host,port]() { impl_->srv.listen(host.c_str(), port); });
  impl_->thr.detach();
}
void RpcServer::set_p2p_receiver(P2PReceiver r){impl_->p2p_receiver=std::move(r);}

void RpcServer::stop() { impl_->srv.stop(); }
}

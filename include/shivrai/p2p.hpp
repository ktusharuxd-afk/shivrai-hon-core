#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "shivrai/types.hpp"
namespace shivrai {
class Blockchain;
class P2P {
public:
  P2P(Blockchain& chain);
  void add_peer(const std::string& url);
  std::vector<std::string> peers();
  void broadcast_block(const Block& b);
  bool receive_block(const Block& b, std::string& reason);
  bool sync_from_peer(const std::string& url);
private:
  Blockchain& chain_;
  std::vector<std::string> peers_;
  std::mutex mu_;
};
}

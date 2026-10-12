#pragma once
#include "shivrai/crypto.hpp"
#include "shivrai/types.hpp"
namespace shivrai {
class Wallet {
public:
  Wallet();
  explicit Wallet(const crypto::KeyPair& kp);
  Transaction create_tx(const std::string& to, uint64_t value, uint64_t nonce,
    const std::string& method="", const json& params=json::object()) const;
  const std::string& address() const { return kp_.address; }
  const crypto::KeyPair& keypair() const { return kp_; }
private:
  crypto::KeyPair kp_;
};
}

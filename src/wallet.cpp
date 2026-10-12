#include "shivrai/wallet.hpp"
#include <chrono>
namespace shivrai {
static uint64_t now_ms() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
Wallet::Wallet() : kp_(crypto::generate_keypair()) {}
Wallet::Wallet(const crypto::KeyPair& kp) : kp_(kp) {}
Transaction Wallet::create_tx(const std::string& to, uint64_t value, uint64_t nonce,
                              const std::string& method, const json& params) const {
  Transaction tx;
  tx.from=kp_.address; tx.to=to; tx.pubkey=kp_.public_key_hex;
  tx.value=value; tx.nonce=nonce;
  tx.timestamp=now_ms(); tx.method=method; tx.params=params;
  std::string payload = tx.from+tx.to+std::to_string(value)+std::to_string(nonce)
    +std::to_string(tx.timestamp)+method+params.dump();
  tx.signature = crypto::sign(kp_.private_key_hex, payload);
  tx.hash = crypto::sha256_hex(payload+tx.signature);
  return tx;
}
}

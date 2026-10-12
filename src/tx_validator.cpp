#include "shivrai/tx_validator.hpp"
#include "shivrai/crypto.hpp"
namespace shivrai {
bool TxValidator::submit(const Transaction& tx, std::string& reason) {
  if (tx.from.empty() || tx.to.empty()) { reason="missing from/to"; return false; }
  if (tx.signature.empty()) { reason="missing signature"; return false; }
  if (tx.pubkey.empty())    { reason="missing pubkey"; return false; }

  // Rebuild signed payload exactly as Wallet::create_tx did
  std::string payload = tx.from + tx.to + std::to_string(tx.value) +
                        std::to_string(tx.nonce) + std::to_string(tx.timestamp) +
                        tx.method + tx.params.dump();

  // 1) ECDSA signature must verify
  if (!crypto::verify(tx.pubkey, payload, tx.signature)) {
    reason = "invalid signature"; return false;
  }
  // 2) Address must derive from pubkey
  std::string derived = "0x" + crypto::sha256_hex(tx.pubkey).substr(0,40);
  if (derived != tx.from) { reason = "pubkey/address mismatch"; return false; }

  // 3) Nonce replay protection
  uint64_t expected = state_.get_u64("nonce:" + tx.from);
  if (tx.nonce != expected) {
    reason = "bad nonce (expected " + std::to_string(expected) +
             ", got " + std::to_string(tx.nonce) + ")";
    return false;
  }
  // 4) Balance covers value + gas
  if (tx.method == "transfer" || tx.method.empty()) {
    uint64_t bal = state_.balance(tx.from);
    if (bal < tx.value + GAS_FEE) { reason = "insufficient balance"; return false; }
  }
  // reserve nonce
  state_.set_u64("nonce:" + tx.from, tx.nonce + 1);
  pool_.add(tx);
  return true;
}
}

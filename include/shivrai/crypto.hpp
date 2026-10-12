#pragma once
#include <string>
#include <vector>
namespace shivrai::crypto {
std::string sha256_hex(const std::string&);
struct KeyPair { std::string private_key_hex, public_key_hex, address; };
KeyPair generate_keypair();
KeyPair keypair_from_priv(const std::string&);
std::string sign(const std::string&, const std::string&);
bool verify(const std::string&, const std::string&, const std::string&);
}

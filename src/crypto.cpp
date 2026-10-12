#include "shivrai/crypto.hpp"
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>
namespace shivrai::crypto {
static std::string to_hex(const unsigned char* b, size_t n) {
  std::ostringstream o;
  for (size_t i=0;i<n;++i) o<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)b[i];
  return o.str();
}
static std::vector<unsigned char> from_hex(const std::string& s) {
  std::vector<unsigned char> o;
  for (size_t i=0;i+1<s.size();i+=2) o.push_back((unsigned char)std::stoi(s.substr(i,2),nullptr,16));
  return o;
}
std::string sha256_hex(const std::string& d) {
  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256((const unsigned char*)d.data(), d.size(), h);
  return to_hex(h, SHA256_DIGEST_LENGTH);
}
static EC_KEY* make_key(const std::string& ph) {
  EC_KEY* k = EC_KEY_new_by_curve_name(NID_secp256k1);
  BIGNUM* bn=nullptr; BN_hex2bn(&bn, ph.c_str());
  if (!bn || !EC_KEY_set_private_key(k,bn)) { BN_free(bn); EC_KEY_free(k); throw std::runtime_error("priv fail"); }
  BN_CTX* c=BN_CTX_new();
  EC_POINT* p=EC_POINT_new(EC_KEY_get0_group(k));
  EC_POINT_mul(EC_KEY_get0_group(k),p,bn,nullptr,nullptr,c);
  EC_KEY_set_public_key(k,p);
  EC_POINT_free(p); BN_CTX_free(c); BN_free(bn);
  return k;
}
KeyPair keypair_from_priv(const std::string& ph) {
  EC_KEY* k = make_key(ph);
  const EC_GROUP* g = EC_KEY_get0_group(k);
  const EC_POINT* p = EC_KEY_get0_public_key(k);
  BN_CTX* c=BN_CTX_new();
  std::vector<unsigned char> b(65);
  size_t n=EC_POINT_point2oct(g,p,POINT_CONVERSION_UNCOMPRESSED,b.data(),b.size(),c);
  std::string pub=to_hex(b.data(),n);
  std::string addr="0x"+sha256_hex(pub).substr(0,40);
  BN_CTX_free(c); EC_KEY_free(k);
  return {ph,pub,addr};
}
KeyPair generate_keypair() {
  unsigned char b[32];
  if (RAND_bytes(b,sizeof(b))!=1) throw std::runtime_error("rand fail");
  return keypair_from_priv(to_hex(b,sizeof(b)));
}
std::string sign(const std::string& ph, const std::string& m) {
  EC_KEY* k=make_key(ph);
  auto hb = from_hex(sha256_hex(m));
  ECDSA_SIG* s = ECDSA_do_sign(hb.data(), hb.size(), k);
  const BIGNUM *r,*ss; ECDSA_SIG_get0(s,&r,&ss);
  char* rh_raw=BN_bn2hex(r); char* sh_raw=BN_bn2hex(ss);
  std::string rh(rh_raw), sh(sh_raw);
  OPENSSL_free(rh_raw); OPENSSL_free(sh_raw);
  // Zero-pad to 64 hex chars (32 bytes) each — leading zeros matter!
  if (rh.size() < 64) rh = std::string(64 - rh.size(), '0') + rh;
  if (sh.size() < 64) sh = std::string(64 - sh.size(), '0') + sh;
  std::string o = rh + sh;
  ECDSA_SIG_free(s); EC_KEY_free(k);
  return o;
}
bool verify(const std::string& pub, const std::string& m, const std::string& sig) {
  auto pb = from_hex(pub);
  EC_KEY* k = EC_KEY_new_by_curve_name(NID_secp256k1);
  const EC_GROUP* g = EC_KEY_get0_group(k);
  EC_POINT* pt = EC_POINT_new(g);
  BN_CTX* c = BN_CTX_new();
  if (!EC_POINT_oct2point(g,pt,pb.data(),pb.size(),c)) {
    EC_POINT_free(pt); BN_CTX_free(c); EC_KEY_free(k); return false;
  }
  EC_KEY_set_public_key(k,pt);
  if (sig.size()%2) { EC_POINT_free(pt); BN_CTX_free(c); EC_KEY_free(k); return false; }
  size_t h=sig.size()/2;
  BIGNUM* r=nullptr; BIGNUM* s=nullptr;
  BN_hex2bn(&r, sig.substr(0,h).c_str());
  BN_hex2bn(&s, sig.substr(h).c_str());
  ECDSA_SIG* es=ECDSA_SIG_new(); ECDSA_SIG_set0(es,r,s);
  auto hb = from_hex(sha256_hex(m));
  int ok = ECDSA_do_verify(hb.data(), hb.size(), es, k);
  ECDSA_SIG_free(es); EC_POINT_free(pt); BN_CTX_free(c); EC_KEY_free(k);
  return ok==1;
}
}

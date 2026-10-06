#ifndef SHIVRAI_HON_INTEROP_MESSAGE_H
#define SHIVRAI_HON_INTEROP_MESSAGE_H
#include <cstdint>
#include <string>
namespace shivrai::interop {
struct Message { std::string source_chain; std::string destination_chain; std::string nonce; std::string payload; };
std::string canonical_encode(const Message& m);
bool validate(const Message& m);
}
#endif

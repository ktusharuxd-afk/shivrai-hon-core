#include "message.h"
namespace shivrai::interop {
std::string canonical_encode(const Message&m){return m.source_chain+"|"+m.destination_chain+"|"+m.nonce+"|"+m.payload;}
bool validate(const Message&m){return !m.source_chain.empty()&&!m.destination_chain.empty()&&!m.nonce.empty()&&!m.payload.empty()&&m.source_chain!=m.destination_chain;}
}

#ifndef SHIVRAI_HON_PAYMENT_H
#define SHIVRAI_HON_PAYMENT_H
#include <cstdint>
#include <string>
namespace shivrai::fintech { struct Payment {std::string id,from,to,currency;uint64_t amount{0};}; bool valid(const Payment&p); }
#endif

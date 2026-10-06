#ifndef SHIVRAI_HON_AMM_H
#define SHIVRAI_HON_AMM_H
#include <cstdint>
#include <utility>
namespace shivrai::defi {
struct Quote { uint64_t amount_out; uint64_t fee; };
class ConstantProductAMM {
public:
    ConstantProductAMM(uint64_t reserve_a, uint64_t reserve_b, uint32_t fee_bps = 30);
    Quote quote(uint64_t amount_in) const;
    bool swap_a_for_b(uint64_t amount_in, uint64_t min_out, uint64_t& out);
    uint64_t reserve_a() const { return a_; }
    uint64_t reserve_b() const { return b_; }
private: uint64_t a_, b_; uint32_t fee_bps_;
};
}
#endif

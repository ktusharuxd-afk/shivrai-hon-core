#include "amm.h"
#include <stdexcept>
#include <limits>
namespace shivrai::defi {
ConstantProductAMM::ConstantProductAMM(uint64_t a,uint64_t b,uint32_t f):a_(a),b_(b),fee_bps_(f){if(!a_||!b_||f>=10000) throw std::invalid_argument("invalid pool");}
Quote ConstantProductAMM::quote(uint64_t in) const { if(!in) return {0,0}; const uint64_t fee=(in*fee_bps_)/10000; const uint64_t net=in-fee; __uint128_t num=(__uint128_t)net*b_; __uint128_t den=(__uint128_t)a_+net; uint64_t out=(uint64_t)(num/den); return {out,fee}; }
bool ConstantProductAMM::swap_a_for_b(uint64_t in,uint64_t min,uint64_t& out){auto q=quote(in); if(q.amount_out<min||q.amount_out>=b_) return false; a_+=in; b_-=q.amount_out; out=q.amount_out; return true;}
}

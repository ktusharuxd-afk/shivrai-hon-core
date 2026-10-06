#include "payment.h"
namespace shivrai::fintech {bool valid(const Payment&p){return !p.id.empty()&&!p.from.empty()&&!p.to.empty()&&p.from!=p.to&&!p.currency.empty()&&p.amount>0;}}

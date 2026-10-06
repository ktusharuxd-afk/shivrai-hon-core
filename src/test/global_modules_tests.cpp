#include <cassert>
#include "shivrai/defi/amm.h"
#include "shivrai/assets/token_ledger.h"
#include "shivrai/assets/rwa.h"
#include "shivrai/identity/identity.h"
#include "shivrai/gaming/assets.h"
#include "shivrai/interop/message.h"
#include "shivrai/fintech/payment.h"
int main(){
 using namespace shivrai;
 defi::ConstantProductAMM pool(100000,100000); auto q=pool.quote(1000); assert(q.amount_out>0); uint64_t out=0; assert(pool.swap_a_for_b(1000,1,out));
 assets::TokenLedger t("SHIV",18); assert(t.mint("alice",1000)); assert(t.transfer("alice","bob",250)); assert(t.balance("bob")==250);
 assets::RWARegistry r; assert(r.register_asset({"r1","issuer","hash","global",false})); assert(r.verify("r1"));
 identity::Registry ids; assert(ids.register_identity({"alice","alice-controller","commit",1,true})); assert(ids.revoke("alice"));
 gaming::Registry g; assert(g.mint({"item1","alice","meta",true})); assert(g.transfer("item1","bob"));
 interop::Message m{"SHIVRAI","ETH","1","payload"}; assert(interop::validate(m)); assert(!interop::validate({"","ETH","1","p"}));
 assert(fintech::valid({"p1","alice","merchant","HON",100})); return 0;
}

import sys, os, time
sys.path.insert(0, os.path.join(os.path.dirname(__file__),"..","..","sdk","python"))
from shivrai import Client
URL="http://127.0.0.1:8545"
for _ in range(40):
    try: Client(URL).health(); break
    except Exception: time.sleep(0.25)
c=Client(URL)
def R(fn,*a,**k):
    last=None
    for _ in range(15):
        try: return fn(*a,**k)
        except Exception as e: last=e; time.sleep(0.3)
    raise last
print("=== shivrai demo (phase 2) ===")
print("health:  ", R(c.health))
print("chain:   ", R(c.chain_info))
a="0xdemo000000000000000000000000000000000000"
print("--- stablecoin ---")
print("mint:    ", R(c.mint_stable,a,1000))
print("balance: ", R(c.stable_balance,a))
print("--- lending ---")
print("deposit: ", R(c.deposit,a,500))
print("position:", R(c.position,a))
print("--- governance ---")
p=R(c.propose,"Increase block size")
print("propose: ", p)
print("vote:    ", R(c.vote,p["proposal_id"]))
print("--- token ---")
print("deploy:  ", R(c.deploy_token,"SHIV",1000000))
print("--- NFT ---")
nft=R(c.nft_mint,a,"ipfs://shivrai/1")
print("mint:    ", nft)
print("owner:   ", R(c.nft_owner,nft["token_id"]))
print("count:   ", R(c.nft_count))
print("--- AMM ---")
R(c.amm_add,"SHIV-ETH",10000,10000)
print("reserves:", R(c.amm_reserves,"SHIV-ETH"))
print("swap:    ", R(c.amm_swap,"SHIV-ETH",100,"a_to_b"))
print("--- staking ---")
print("stake:   ", R(c.stake,a,500))
print("info:    ", R(c.stake_info,a))
print("--- multisig ---")
ms=R(c.ms_create,[a,"0xbob"],2)
print("create:  ", ms)
print("sign1:   ", R(c.ms_sign,ms["wallet_id"]))
print("sign2:   ", R(c.ms_sign,ms["wallet_id"]))
print("--- explorer ---")
print("stats:   ", R(c.explorer_stats))
print("done")

import json
from urllib.request import Request, urlopen
__version__ = "1.0.0"
class Client:
    def __init__(self, url="http://127.0.0.1:8545"):
        self.url = url.rstrip("/"); self._id = 0
    def health(self):
        with urlopen(self.url+"/health") as r: return json.loads(r.read().decode())
    def call(self, method, params=None):
        self._id += 1
        payload = json.dumps({"jsonrpc":"2.0","id":self._id,"method":method,"params":params or {}}).encode()
        req = Request(self.url+"/rpc", data=payload, headers={"Content-Type":"application/json"})
        with urlopen(req) as r: resp = json.loads(r.read().decode())
        if "error" in resp: raise RuntimeError(resp["error"])
        return resp["result"]
    def chain_info(self): return self.call("chain_info")
    def balance(self, a): return self.call("account_balance", {"address":a})
    def mint_stable(self, a, n): return self.call("stablecoin_mint", {"address":a,"amount":n})
    def stable_balance(self, a): return self.call("stablecoin_balance", {"address":a})
    def deposit(self, a, n): return self.call("lending_deposit", {"address":a,"amount":n})
    def borrow(self, a, n): return self.call("lending_borrow", {"address":a,"amount":n})
    def position(self, a): return self.call("lending_position", {"address":a})
    def propose(self, t): return self.call("governance_propose", {"title":t})
    def vote(self, p): return self.call("governance_vote", {"proposal_id":p})
    def result(self, p): return self.call("governance_result", {"proposal_id":p})
    def deploy_token(self, s, sup): return self.call("token_deploy", {"symbol":s,"supply":sup})
    def token_info(self, t): return self.call("token_info", {"token_id":t})

def _extend():
    def nft_mint(self,o,u): return self.call("nft_mint",{"owner":o,"uri":u})
    def nft_owner(self,t): return self.call("nft_ownerOf",{"token_id":t})
    def nft_transfer(self,t,f,to): return self.call("nft_transfer",{"token_id":t,"from":f,"to":to})
    def nft_count(self): return self.call("nft_count")
    def amm_add(self,p,a,b): return self.call("amm_addLiquidity",{"pool":p,"amount_a":a,"amount_b":b})
    def amm_reserves(self,p): return self.call("amm_reserves",{"pool":p})
    def amm_swap(self,p,a,d): return self.call("amm_swap",{"pool":p,"amount_in":a,"direction":d})
    def stake(self,a,n): return self.call("staking_stake",{"address":a,"amount":n})
    def unstake(self,a,n): return self.call("staking_unstake",{"address":a,"amount":n})
    def stake_info(self,a): return self.call("staking_info",{"address":a})
    def ms_create(self,o,t): return self.call("multisig_create",{"owners":o,"threshold":t})
    def ms_sign(self,w): return self.call("multisig_sign",{"wallet_id":w})
    def ms_info(self,w): return self.call("multisig_info",{"wallet_id":w})
    def explorer_stats(self): return self.call("explorer_stats")
    for k,v in locals().items():
        if callable(v) and k not in ("self",): setattr(Client,k,v)
_extend()

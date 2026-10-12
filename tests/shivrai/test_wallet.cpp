#include "shivrai/wallet/wallet.h"
#include "shivrai/wallet/mnemonic.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace shivrai;
using namespace shivrai::wallet;

int main() {
    // Wallet basics
    ShivraiWallet w("test");
    assert(w.name() == "test");
    assert(!w.IsEncrypted());

    // HD init
    std::vector<uint8_t> seed(32, 0x42);
    assert(w.InitHD(seed));
    assert(w.hd().IsInitialized());

    // Addresses
    auto a1 = w.NewAddress("alice");
    auto a2 = w.NewAddress("bob");
    assert(a1 && a2);
    assert(*a1 != *a2);
    assert(a1->substr(0, 4) == "hon1");

    auto a3 = w.NewAddress("legacy", false);
    assert(a3 && (*a3)[0] == 'H');

    auto c1 = w.ChangeAddress();
    assert(c1 && *c1 != *a1);

    // Decode
    std::vector<uint8_t> script;
    assert(w.DecodeAddress(*a1, script));
    assert(script.size() == 20);

    // Balance + send
    assert(w.GetBalance().total() == 0);
    w.FundAddress(*a1, 1000000);
    assert(w.GetBalance().total() == 1000000);

    auto txid = w.SendToAddress(*a2, 500000);
    assert(txid);
    assert(w.TxCount() == 1);

    auto tx = w.GetTransaction(*txid);
    assert(tx && tx->amount == 500000 && tx->fee == 1000);

    auto bad = w.SendToAddress(*a2, 999999999);
    assert(!bad);

    // Multi-asset
    w.assets().Credit(*a1, std::string(64, 'a'), 500);
    assert(w.GetAssetBalance(*a1, std::string(64, 'a')) == 500);

    // NFT
    assert(w.MintNft(std::string(64, 'n'), 1, *a1, "ipfs://x"));
    assert(w.NftsOf(*a1).size() == 1);

    std::string err;
    assert(w.SendNft(std::string(64, 'n'), 1, *a2, err));
    assert(w.NftsOf(*a2).size() == 1);

    // Encryption
    Argon2Params kp; kp.t_cost = 1; kp.m_cost = 64; kp.parallelism = 1;
    w.keystore().SetKDFParams(kp);
    assert(w.EncryptWallet("pw"));
    assert(w.IsEncrypted() && w.IsLocked());
    assert(!w.Unlock("bad", 60));
    assert(w.Unlock("pw", 60));
    w.Lock();
    assert(w.IsLocked());

    // Mnemonic
    ShivraiWallet w2("w2");
    auto m = mnemonic::Generate(128);
    assert(w2.ImportMnemonic(m, ""));
    assert(w2.DumpMnemonic("") == m);

    std::cout << "========================================\n";
    std::cout << "  ALL WALLET TESTS PASSED\n";
    std::cout << "========================================\n";
    return 0;
}

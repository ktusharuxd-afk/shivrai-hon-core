#include "shivrai/wrapped_tokens/wrapped_tokens.h"
#include <cstdlib>
#include <iostream>
using namespace shivrai::wrapped_tokens;
int main() {
    int passed = 0;
#define CHECK(x) do { ++passed; if (!(x)) { std::cerr << "FAIL " << __LINE__ << "\n"; std::exit(1); } } while(0)
    WrappedTokens wt;
    CHECK(wt.AssetCount() == 0);
    CHECK(!wt.Register("", "eth", "ETH"));
    CHECK(!wt.Register("WETH", "", "ETH"));
    CHECK(!wt.Register("WETH", "eth", ""));
    CHECK(wt.Register("WETH", "ethereum", "ETH"));
    CHECK(!wt.Register("WETH", "eth", "ETH"));
    CHECK(wt.AssetCount() == 1);
    auto a = wt.Get("WETH");
    CHECK(a.has_value());
    CHECK(a->underlying_chain == "ethereum");
    CHECK(a->underlying_symbol == "ETH");
    CHECK(a->total_supply == 0);
    CHECK(wt.IsFullyBacked("WETH"));
    CHECK(!wt.Wrap("NOPE", "alice", 100));
    CHECK(!wt.Wrap("WETH", "", 100));
    CHECK(!wt.Wrap("WETH", "alice", 0));
    CHECK(wt.Wrap("WETH", "alice", 1000));
    CHECK(wt.Wrap("WETH", "bob", 500));
    CHECK(wt.BalanceOf("WETH", "alice") == 1000);
    CHECK(wt.BalanceOf("WETH", "bob") == 500);
    CHECK(wt.BalanceOf("WETH", "carol") == 0);
    CHECK(wt.TotalSupply("WETH") == 1500);
    CHECK(wt.TotalBacking("WETH") == 1500);
    CHECK(wt.IsFullyBacked("WETH"));
    auto u = wt.Unwrap("WETH", "alice", 400);
    CHECK(u.has_value() && *u == 400);
    CHECK(wt.BalanceOf("WETH", "alice") == 600);
    CHECK(wt.TotalSupply("WETH") == 1100);
    CHECK(!wt.Unwrap("WETH", "alice", 99999).has_value());
    CHECK(!wt.Unwrap("NOPE", "alice", 100).has_value());
    CHECK(!wt.Unwrap("WETH", "alice", 0).has_value());
    CHECK(wt.SetActive("WETH", false));
    CHECK(!wt.Wrap("WETH", "carol", 100));
    CHECK(wt.SetActive("WETH", true));
    CHECK(wt.Wrap("WETH", "carol", 100));
    CHECK(!wt.SetActive("NOPE", false));
    CHECK(!wt.Get("NOPE").has_value());
    CHECK(wt.TotalSupply("NOPE") == 0);
    CHECK(wt.TotalBacking("NOPE") == 0);
    CHECK(!wt.IsFullyBacked("NOPE"));
    std::cout << "All " << passed << " assertions passed\n";
    return 0;
}

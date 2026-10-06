// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/genesis.h>

namespace shivrai {

const GenesisSpec& GlobalGenesisSpec()
{
    static const GenesisSpec spec{
        "shivrai-mainnet-v2",
        "SHIVRAI HON GLOBAL GENESIS | Decentralized payments, finance, gaming and open blockchain infrastructure",
        0,          // Freeze at launch ceremony time.
        0,          // Determined by the genesis PoW mining ceremony.
        0x1d00ffff, // Candidate difficulty; must be recalibrated for launch security.
        1,
        2000,
        false,
    };
    return spec;
}

} // namespace shivrai

# 🚀 Shivrai Hon Core

Production-style C++ blockchain module library with HTTP JSON-RPC node and Python SDK.

## 📦 What's Inside

- **29 modules** — DeFi, NFTs, governance, staking, bridges, gaming
- **1400+ test assertions** — every module fully tested
- **HTTP JSON-RPC node** — REST API on port 8545
- **Python SDK** — simple client
- **CMake build** — one command
- **Docker** — containerized
- **Examples** — Python and curl demos

## 🏗️ Modules

| Module |
|--------|
| stablecoin |
| lending |
| nft_platform |
| marketplace |
| staking |
| governance |
| gaming |
| multisig |
| bridge |
| dex |
| yield_farm |
| voting_escrow |
| atomic_swap |
| lottery |
| fee_market |
| randomness_beacon |
| wrapped_tokens |
| kyc_registry |
| ticket_system |
| subscription |
| dao |
| oracle_aggregator |
| health_monitor |
| airdrop |
| upgradeable_proxy |
| indexer |
| timelock |
| analytics |
| event_log |
| payment_stream |

## 🚀 Quick Start

```bash
bash scripts/build.sh
```

Then in a second terminal:

```bash
bash scripts/run_node.sh
```

Test it:

```bash
bash examples/curl/demo.sh
python3 examples/python/demo.py
```

## 📡 API

- `GET /health` — simple health check
- `POST /rpc` — JSON-RPC 2.0

Methods: `system.health`, `system.info`, `system.echo`

## 🐍 Python SDK

```python
from sdk.python.shivrai_client import ShivraiClient
c = ShivraiClient("http://localhost:8545")
print(c.health())
print(c.echo("hello"))
```

## 🐳 Docker

```bash
docker build -t shivrai-node .
docker run -p 8545:8545 shivrai-node
```

## 🧪 Tests

```bash
bash scripts/run_tests.sh
```

## 📄 License

MIT

## 🔗 Repository

https://github.com/ktusharuxd-afk/shivrai-hon-core

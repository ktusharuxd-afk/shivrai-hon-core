// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/evm_state.h>

#include <algorithm>
#include <limits>

namespace shivrai::evm {

Account& StateDB::GetOrCreate(const Address& address)
{
    return accounts_[address];
}

const Account* StateDB::Get(const Address& address) const
{
    const auto it = accounts_.find(address);
    return it == accounts_.end() ? nullptr : &it->second;
}

bool StateDB::Exists(const Address& address) const
{
    return accounts_.find(address) != accounts_.end();
}

uint64_t StateDB::GetBalance(const Address& address) const
{
    const auto* acc = Get(address);
    return acc ? acc->balance_hon_nano : 0;
}

void StateDB::SetBalance(const Address& address, uint64_t amount)
{
    accounts_[address].balance_hon_nano = amount;
}

uint64_t StateDB::GetNonce(const Address& address) const
{
    const auto* acc = Get(address);
    return acc ? acc->nonce : 0;
}

void StateDB::SetNonce(const Address& address, uint64_t nonce)
{
    accounts_[address].nonce = nonce;
}

void StateDB::IncrementNonce(const Address& address)
{
    accounts_[address].nonce++;
}

const std::vector<uint8_t>& StateDB::GetCode(const Address& address) const
{
    static const std::vector<uint8_t> empty;
    const auto* acc = Get(address);
    return acc ? acc->code : empty;
}

void StateDB::SetCode(const Address& address, const std::vector<uint8_t>& code)
{
    accounts_[address].code = code;
}

Word StateDB::GetStorage(const Address& address, const Word& key) const
{
    const auto* acc = Get(address);
    if (!acc) return Word{};
    const auto it = acc->storage.find(key);
    return it == acc->storage.end() ? Word{} : it->second;
}

void StateDB::SetStorage(const Address& address, const Word& key, const Word& value)
{
    accounts_[address].storage[key] = value;
}

bool StateDB::Transfer(const Address& from, const Address& to,
                       uint64_t amount, std::string& error)
{
    auto& src = accounts_[from];
    auto& dst = accounts_[to];
    if (src.balance_hon_nano < amount) {
        error = "insufficient HON balance";
        return false;
    }
    if (std::numeric_limits<uint64_t>::max() - dst.balance_hon_nano < amount) {
        error = "recipient balance overflow";
        return false;
    }
    src.balance_hon_nano -= amount;
    dst.balance_hon_nano += amount;
    return true;
}

uint64_t StateDB::StateHash() const
{
    uint64_t h = 1469598103934665603ULL;
    for (const auto& [addr, acc] : accounts_) {
        for (auto b : addr) {
            h ^= b;
            h *= 1099511628211ULL;
        }
        h ^= acc.balance_hon_nano;
        h *= 1099511628211ULL;
        h ^= acc.nonce;
        h *= 1099511628211ULL;
    }
    return h;
}

// ═══════════════════════════════════════════════════════
// Execution kernel — Phase 5
// ═══════════════════════════════════════════════════════

namespace {

constexpr uint8_t OP_STOP   = 0x00;
constexpr uint8_t OP_ADD    = 0x01;
constexpr uint8_t OP_MUL    = 0x02;
constexpr uint8_t OP_SUB    = 0x03;
constexpr uint8_t OP_DIV    = 0x04;
constexpr uint8_t OP_MOD    = 0x06;
constexpr uint8_t OP_LT     = 0x10;
constexpr uint8_t OP_GT     = 0x11;
constexpr uint8_t OP_EQ     = 0x14;
constexpr uint8_t OP_ISZERO = 0x15;
constexpr uint8_t OP_AND    = 0x16;
constexpr uint8_t OP_OR     = 0x17;
constexpr uint8_t OP_XOR    = 0x18;
constexpr uint8_t OP_NOT    = 0x19;
constexpr uint8_t OP_POP    = 0x50;
constexpr uint8_t OP_PUSH1  = 0x60;
constexpr uint8_t OP_DUP1   = 0x80;
constexpr uint8_t OP_SWAP1  = 0x90;
constexpr uint8_t OP_RETURN = 0xf3;
constexpr uint8_t OP_REVERT = 0xfd;

uint64_t WordToU64(const Word& w)
{
    uint64_t out = 0;
    for (size_t i = 24; i < 32; ++i) out = (out << 8) | w[i];
    return out;
}

Word U64ToWord(uint64_t v)
{
    Word w{};
    for (size_t i = 0; i < 8; ++i) w[31 - i] = static_cast<uint8_t>(v >> (i * 8));
    return w;
}

bool Charge(uint64_t& gas, uint64_t cost, std::string& error)
{
    if (gas < cost) {
        error = "out of gas";
        return false;
    }
    gas -= cost;
    return true;
}

} // anonymous namespace

ExecutionResult ExecuteBytecode(const std::vector<uint8_t>& code,
                                uint64_t gas_limit,
                                const std::vector<uint8_t>& input)
{
    (void)input;
    ExecutionResult r;
    std::vector<Word> stack;
    std::vector<uint8_t> memory;
    uint64_t gas = gas_limit;
    size_t pc = 0;

    while (pc < code.size()) {
        const uint8_t op = code[pc++];
        const uint64_t cost = (op == OP_PUSH1 ? 3 : 2);
        if (!Charge(gas, cost, r.error)) {
            r.gas_used = gas_limit - gas;
            return r;
        }

        if (op == OP_STOP) {
            r.success = true;
            r.gas_used = gas_limit - gas;
            return r;
        }

        if (op == OP_PUSH1) {
            if (pc >= code.size()) {
                r.error = "truncated PUSH1";
                r.gas_used = gas_limit - gas;
                return r;
            }
            Word w{};
            w[31] = code[pc++];
            stack.push_back(w);
            continue;
        }

        if (op == OP_POP) {
            if (stack.empty()) {
                r.error = "stack underflow";
                r.gas_used = gas_limit - gas;
                return r;
            }
            stack.pop_back();
            continue;
        }

        if (op == OP_DUP1) {
            if (stack.empty()) {
                r.error = "stack underflow (DUP1)";
                r.gas_used = gas_limit - gas;
                return r;
            }
            stack.push_back(stack.back());
            continue;
        }

        if (op == OP_SWAP1) {
            if (stack.size() < 2) {
                r.error = "stack underflow (SWAP1)";
                r.gas_used = gas_limit - gas;
                return r;
            }
            std::swap(stack[stack.size() - 1], stack[stack.size() - 2]);
            continue;
        }

        if (op == OP_ISZERO || op == OP_NOT) {
            if (stack.empty()) {
                r.error = "stack underflow";
                r.gas_used = gas_limit - gas;
                return r;
            }
            const uint64_t a = WordToU64(stack.back());
            stack.pop_back();
            uint64_t v = (op == OP_ISZERO) ? (a == 0 ? 1 : 0) : ~a;
            stack.push_back(U64ToWord(v));
            continue;
        }

        if (op == OP_ADD || op == OP_MUL || op == OP_SUB ||
            op == OP_DIV || op == OP_MOD ||
            op == OP_LT || op == OP_GT || op == OP_EQ ||
            op == OP_AND || op == OP_OR || op == OP_XOR) {
            if (stack.size() < 2) {
                r.error = "stack underflow";
                r.gas_used = gas_limit - gas;
                return r;
            }
            const uint64_t a = WordToU64(stack.back()); stack.pop_back();
            const uint64_t b = WordToU64(stack.back()); stack.pop_back();
            uint64_t v = 0;
            switch (op) {
                case OP_ADD: v = b + a; break;
                case OP_MUL: v = b * a; break;
                case OP_SUB: v = b - a; break;
                case OP_DIV: v = (a == 0) ? 0 : b / a; break;
                case OP_MOD: v = (a == 0) ? 0 : b % a; break;
                case OP_LT:  v = (b < a) ? 1 : 0; break;
                case OP_GT:  v = (b > a) ? 1 : 0; break;
                case OP_EQ:  v = (b == a) ? 1 : 0; break;
                case OP_AND: v = b & a; break;
                case OP_OR:  v = b | a; break;
                case OP_XOR: v = b ^ a; break;
            }
            stack.push_back(U64ToWord(v));
            continue;
        }

        if (op == OP_RETURN || op == OP_REVERT) {
            if (stack.size() < 2) {
                r.error = "stack underflow";
                r.gas_used = gas_limit - gas;
                return r;
            }
            const uint64_t offset = WordToU64(stack.back()); stack.pop_back();
            const uint64_t size = WordToU64(stack.back()); stack.pop_back();
            if (offset > 1'000'000 || size > 1'000'000 ||
                offset + size > 1'000'000) {
                r.error = "memory bounds exceeded";
                r.gas_used = gas_limit - gas;
                return r;
            }
            if (memory.size() < offset + size) memory.resize(offset + size, 0);
            r.return_data.assign(memory.begin() + offset, memory.begin() + offset + size);
            r.reverted = (op == OP_REVERT);
            r.success = (op == OP_RETURN);
            r.gas_used = gas_limit - gas;
            return r;
        }

        r.error = "unsupported opcode";
        r.gas_used = gas_limit - gas;
        return r;
    }

    r.success = true;
    r.gas_used = gas_limit - gas;
    return r;
}

} // namespace shivrai::evm

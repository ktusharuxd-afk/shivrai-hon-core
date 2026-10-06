// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.
#include <shivrai/evm_state.h>

#include <algorithm>
#include <limits>

namespace shivrai::evm {

Account& StateDB::GetOrCreate(const Address& address) { return accounts_[address]; }

const Account* StateDB::Get(const Address& address) const
{
    const auto it = accounts_.find(address);
    return it == accounts_.end() ? nullptr : &it->second;
}

bool StateDB::Transfer(const Address& from, const Address& to, uint64_t amount, std::string& error)
{
    auto& src = accounts_[from];
    auto& dst = accounts_[to];
    if (src.balance_hon_nano < amount) {
        error = "insufficient HON balance";
        return false;
    }
    src.balance_hon_nano -= amount;
    if (std::numeric_limits<uint64_t>::max() - dst.balance_hon_nano < amount) {
        error = "recipient balance overflow";
        src.balance_hon_nano += amount;
        return false;
    }
    dst.balance_hon_nano += amount;
    return true;
}

namespace {
constexpr uint8_t STOP = 0x00;
constexpr uint8_t ADD = 0x01;
constexpr uint8_t MUL = 0x02;
constexpr uint8_t SUB = 0x03;
constexpr uint8_t POP = 0x50;
constexpr uint8_t PUSH1 = 0x60;
constexpr uint8_t RETURN = 0xf3;
constexpr uint8_t REVERT = 0xfd;

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
    if (gas < cost) { error = "out of gas"; return false; }
    gas -= cost;
    return true;
}
}

ExecutionResult ExecuteBytecode(const std::vector<uint8_t>& code, uint64_t gas_limit,
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
        const uint64_t cost = (op == PUSH1 ? 3 : 2);
        if (!Charge(gas, cost, r.error)) { r.gas_used = gas_limit - gas; return r; }

        if (op == STOP) { r.success = true; r.gas_used = gas_limit - gas; return r; }
        if (op == PUSH1) {
            if (pc >= code.size()) { r.error = "truncated PUSH1"; r.gas_used = gas_limit - gas; return r; }
            Word w{}; w[31] = code[pc++]; stack.push_back(w); continue;
        }
        if (op == POP) {
            if (stack.empty()) { r.error = "stack underflow"; r.gas_used = gas_limit - gas; return r; }
            stack.pop_back(); continue;
        }
        if (op == ADD || op == MUL || op == SUB) {
            if (stack.size() < 2) { r.error = "stack underflow"; r.gas_used = gas_limit - gas; return r; }
            const uint64_t a = WordToU64(stack.back()); stack.pop_back();
            const uint64_t b = WordToU64(stack.back()); stack.pop_back();
            uint64_t v = 0;
            if (op == ADD) v = b + a;
            else if (op == MUL) v = b * a;
            else v = b - a;
            stack.push_back(U64ToWord(v));
            continue;
        }
        if (op == RETURN || op == REVERT) {
            if (stack.size() < 2) { r.error = "stack underflow"; r.gas_used = gas_limit - gas; return r; }
            const uint64_t offset = WordToU64(stack.back()); stack.pop_back();
            const uint64_t size = WordToU64(stack.back()); stack.pop_back();
            if (offset > 1'000'000 || size > 1'000'000 || offset + size > 1'000'000) {
                r.error = "memory bounds exceeded"; r.gas_used = gas_limit - gas; return r;
            }
            if (memory.size() < offset + size) memory.resize(offset + size, 0);
            r.return_data.assign(memory.begin() + offset, memory.begin() + offset + size);
            r.reverted = (op == REVERT);
            r.success = (op == RETURN);
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

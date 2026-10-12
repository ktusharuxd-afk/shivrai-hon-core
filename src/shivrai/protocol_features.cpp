// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.

#include "shivrai/protocol_features.h"

namespace shivrai {

const std::vector<ProtocolFeature>& GlobalProtocolFeatures()
{
    static const std::vector<ProtocolFeature> features = {
        // Foundation modules (implemented + tested)
        {"identity",        "Decentralized Identity",   "identity",  FeatureStatus::FOUNDATION, false},
        {"interop",         "Cross-Chain Messaging",    "interop",   FeatureStatus::FOUNDATION, false},
        {"rwa",             "Real-World Assets",        "assets",    FeatureStatus::FOUNDATION, false},
        {"token_ledger",    "Native Token Standard",    "assets",    FeatureStatus::FOUNDATION, false},
        {"gaming",          "Gaming Assets",            "gaming",    FeatureStatus::FOUNDATION, false},
        {"payment",         "Payment Processor",        "fintech",   FeatureStatus::FOUNDATION, false},
        {"amm",             "Automated Market Maker",   "defi",      FeatureStatus::FOUNDATION, false},
        {"treasury",        "Treasury & Multisig",      "treasury",  FeatureStatus::FOUNDATION, false},
        {"monetary_policy", "Monetary Policy",          "consensus", FeatureStatus::FOUNDATION, false},
        {"genesis",         "Genesis Specification",    "consensus", FeatureStatus::FOUNDATION, false},
        {"evm_compat",      "EVM Compatibility Layer",  "evm",       FeatureStatus::FOUNDATION, false},
        {"evm_state",       "EVM State Kernel",         "evm",       FeatureStatus::FOUNDATION, false},

        // Planned modules
        {"wallet",          "Wallet (Mobile/Desktop)",  "tooling",   FeatureStatus::PLANNED, false},
        {"nft",             "NFT Platform",             "assets",    FeatureStatus::PLANNED, false},
        {"stablecoin",      "Stablecoin System",        "fintech",   FeatureStatus::PLANNED, false},
        {"lending",         "Lending & Borrowing",      "defi",      FeatureStatus::PLANNED, false},
        {"oracle",          "Oracle Network",           "data",      FeatureStatus::PLANNED, false},
        {"governance",      "Governance System",        "protocol",  FeatureStatus::PLANNED, false},
    };
    return features;
}

const char* FeatureStatusName(FeatureStatus status)
{
    switch (status) {
        case FeatureStatus::FOUNDATION: return "foundation";
        case FeatureStatus::PLANNED:    return "planned";
        case FeatureStatus::ACTIVE:     return "active";
        case FeatureStatus::DEPRECATED: return "deprecated";
    }
    return "unknown";
}

// ─── FeatureRegistry ───

bool FeatureRegistry::register_feature(const std::string& id,
                                        const std::string& name,
                                        const std::string& domain,
                                        FeatureStatus status,
                                        bool ai_enabled)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (id.empty() || name.empty()) return false;
    if (features_.count(id)) return false;
    FeatureState s;
    s.id = id;
    s.name = name;
    s.domain = domain;
    s.status = status;
    s.ai_enabled = ai_enabled;
    s.active = (status == FeatureStatus::ACTIVE);
    features_[id] = s;
    return true;
}

bool FeatureRegistry::activate(const std::string& id, uint64_t height)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = features_.find(id);
    if (it == features_.end()) return false;
    if (it->second.active) return false;
    it->second.active = true;
    it->second.status = FeatureStatus::ACTIVE;
    it->second.activation_height = height;
    return true;
}

bool FeatureRegistry::deactivate(const std::string& id)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = features_.find(id);
    if (it == features_.end()) return false;
    if (!it->second.active) return false;
    it->second.active = false;
    it->second.status = FeatureStatus::PLANNED;
    return true;
}

bool FeatureRegistry::is_active(const std::string& id) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = features_.find(id);
    return it != features_.end() && it->second.active;
}

bool FeatureRegistry::has_feature(const std::string& id) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return features_.count(id) > 0;
}

const FeatureState* FeatureRegistry::find(const std::string& id) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = features_.find(id);
    return it == features_.end() ? nullptr : &it->second;
}

std::vector<FeatureState> FeatureRegistry::list() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<FeatureState> result;
    result.reserve(features_.size());
    for (const auto& [id, s] : features_) result.push_back(s);
    return result;
}

size_t FeatureRegistry::size() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return features_.size();
}

size_t FeatureRegistry::active_count() const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    size_t n = 0;
    for (const auto& [id, s] : features_) if (s.active) n++;
    return n;
}

std::vector<FeatureState> FeatureRegistry::by_domain(const std::string& domain) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<FeatureState> result;
    for (const auto& [id, s] : features_) {
        if (s.domain == domain) result.push_back(s);
    }
    return result;
}

} // namespace shivrai

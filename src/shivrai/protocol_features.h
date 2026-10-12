// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.

#ifndef SHIVRAI_HON_PROTOCOL_FEATURES_H
#define SHIVRAI_HON_PROTOCOL_FEATURES_H

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace shivrai {

enum class FeatureStatus {
    FOUNDATION,
    PLANNED,
    ACTIVE,
    DEPRECATED,
};

struct ProtocolFeature {
    const char* id;
    const char* name;
    const char* domain;
    FeatureStatus status;
    bool ai_enabled;
};

// Static list of protocol features (built-in registry).
const std::vector<ProtocolFeature>& GlobalProtocolFeatures();

// Human-readable status name.
const char* FeatureStatusName(FeatureStatus status);

// ─── Runtime Feature Registry (dynamic) ───

struct FeatureState {
    std::string id;
    std::string name;
    std::string domain;
    FeatureStatus status{FeatureStatus::PLANNED};
    bool ai_enabled{false};
    uint64_t activation_height{0};
    bool active{false};
};

class FeatureRegistry {
public:
    FeatureRegistry() = default;

    // Register a new feature. Returns false if id already exists or empty.
    bool register_feature(const std::string& id,
                          const std::string& name,
                          const std::string& domain,
                          FeatureStatus status = FeatureStatus::PLANNED,
                          bool ai_enabled = false);

    // Activate a registered feature at a given block height.
    bool activate(const std::string& id, uint64_t height = 0);

    // Deactivate (for testing / rollback).
    bool deactivate(const std::string& id);

    // Query
    bool is_active(const std::string& id) const;
    bool has_feature(const std::string& id) const;
    const FeatureState* find(const std::string& id) const;
    std::vector<FeatureState> list() const;

    // Counts
    size_t size() const;
    size_t active_count() const;

    // Filter by domain
    std::vector<FeatureState> by_domain(const std::string& domain) const;

private:
    mutable std::recursive_mutex mutex_;
    std::unordered_map<std::string, FeatureState> features_;
};

} // namespace shivrai

#endif // SHIVRAI_HON_PROTOCOL_FEATURES_H

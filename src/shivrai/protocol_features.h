// Copyright (c) 2026-present The SHIVRAI HON Core developers
// Distributed under the MIT software license.

#ifndef SHIVRAI_HON_PROTOCOL_FEATURES_H
#define SHIVRAI_HON_PROTOCOL_FEATURES_H

#include <string>
#include <vector>

namespace shivrai {

enum class FeatureStatus {
    FOUNDATION,
    PLANNED,
};

struct ProtocolFeature {
    const char* id;
    const char* name;
    const char* domain;
    FeatureStatus status;
    bool ai_enabled;
};

const std::vector<ProtocolFeature>& GlobalProtocolFeatures();
const char* FeatureStatusName(FeatureStatus status);

} // namespace shivrai

#endif // SHIVRAI_HON_PROTOCOL_FEATURES_H

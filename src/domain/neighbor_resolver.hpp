#pragma once

#include <functional>
#include <optional>

#include "domain/types.hpp"

namespace morph_bridge {

using FindObjectAt = std::function<std::optional<ObjectSpan>(int layer, int frame)>;

std::optional<EndpointPair> resolve_neighbors(const ObjectSpan& bridge,
                                              const FindObjectAt& find_at);

}  // namespace morph_bridge

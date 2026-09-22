#include "domain/neighbor_resolver.hpp"

#include <limits>

namespace morph_bridge {
namespace {

bool is_before(const ObjectSpan& candidate, const ObjectSpan& bridge) {
  return candidate.token != bridge.token && candidate.layer == bridge.layer &&
         candidate.start < bridge.start && candidate.end < bridge.start;
}

bool is_after(const ObjectSpan& candidate, const ObjectSpan& bridge) {
  if (candidate.token == bridge.token || candidate.layer != bridge.layer) {
    return false;
  }
  if (candidate.start < bridge.end) return false;
  if (bridge.end == std::numeric_limits<int>::max()) {
    return candidate.start == bridge.end;
  }
  return candidate.start <= bridge.end + 1;
}

}  // namespace

std::optional<EndpointPair> resolve_neighbors(const ObjectSpan& bridge,
                                              const FindObjectAt& find_at) {
  if (!find_at || bridge.start <= 0 || bridge.end < bridge.start) {
    return std::nullopt;
  }

  const auto before = find_at(bridge.layer, bridge.start - 1);
  if (!before || !is_before(*before, bridge)) return std::nullopt;

  std::optional<ObjectSpan> after = find_at(bridge.layer, bridge.end);
  if ((!after || !is_after(*after, bridge)) &&
      bridge.end < std::numeric_limits<int>::max()) {
    after = find_at(bridge.layer, bridge.end + 1);
  }
  if (!after || !is_after(*after, bridge)) return std::nullopt;

  return EndpointPair{*before, *after, before->end, after->start};
}

}  // namespace morph_bridge

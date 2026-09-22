#include "render/sdf_plan.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace morph_bridge {

std::optional<SdfDispatchPlan> build_sdf_plan(
    const int width, const int height, std::wstring resource_prefix) {
  if (width <= 0 || height <= 0 || resource_prefix.empty() ||
      width > 65'534 || height > 65'534) {
    return std::nullopt;
  }

  SdfDispatchPlan plan;
  plan.width = width;
  plan.height = height;
  plan.groups_x = (width + 7) / 8;
  plan.groups_y = (height + 7) / 8;
  plan.upload = resource_prefix + L".upload";
  plan.seed = resource_prefix + L".seed";
  plan.ping = resource_prefix + L".ping";
  plan.pong = resource_prefix + L".pong";
  plan.final_sdf = resource_prefix + L".sdf";

  int enclosing_power = 1;
  const int longest_axis = std::max(width, height);
  while (enclosing_power < longest_axis &&
         enclosing_power <= std::numeric_limits<int>::max() / 2) {
    enclosing_power *= 2;
  }

  std::wstring source = plan.seed;
  bool use_ping = true;
  for (int step = enclosing_power / 2; step >= 1; step /= 2) {
    const std::wstring& target = use_ping ? plan.ping : plan.pong;
    plan.jumps.push_back({step, source, target});
    source = target;
    use_ping = !use_ping;
  }
  plan.final_seed = source;
  return plan;
}

}  // namespace morph_bridge

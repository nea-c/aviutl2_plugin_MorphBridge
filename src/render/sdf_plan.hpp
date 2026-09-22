#pragma once

#include <optional>
#include <string>
#include <vector>

namespace morph_bridge {

struct SdfJumpPass {
  int step{};
  std::wstring source;
  std::wstring target;
  friend bool operator==(const SdfJumpPass&, const SdfJumpPass&) = default;
};

struct SdfDispatchPlan {
  std::wstring prefix;
  int width{};
  int height{};
  int groups_x{};
  int groups_y{};
  std::wstring upload;
  std::wstring seed;
  std::wstring ping;
  std::wstring pong;
  std::wstring final_seed;
  std::wstring final_sdf;
  std::vector<SdfJumpPass> jumps;
  friend bool operator==(const SdfDispatchPlan&, const SdfDispatchPlan&) = default;
};

[[nodiscard]] std::optional<SdfDispatchPlan> build_sdf_plan(
    int width, int height, std::wstring resource_prefix);

}  // namespace morph_bridge

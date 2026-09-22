#include "render/sdf_plan.hpp"
#include "test_support.hpp"

#include <array>

using morph_bridge::build_sdf_plan;

void run_sdf_plan_tests() {
  const auto plan = build_sdf_plan(320, 180, L"cache:morph.a");
  MB_CHECK(plan.has_value());
  MB_CHECK(plan->groups_x == 40);
  MB_CHECK(plan->groups_y == 23);
  constexpr std::array expected_steps{256, 128, 64, 32, 16, 8, 4, 2, 1};
  MB_CHECK(plan->jumps.size() == expected_steps.size());
  for (std::size_t index = 0; index < expected_steps.size(); ++index) {
    MB_CHECK(plan->jumps[index].step == expected_steps[index]);
    MB_CHECK(plan->jumps[index].source != plan->jumps[index].target);
    if (index != 0) {
      MB_CHECK(plan->jumps[index].source == plan->jumps[index - 1].target);
    }
  }
  MB_CHECK(plan->final_seed == plan->jumps.back().target);
  MB_CHECK(plan->final_sdf == L"cache:morph.a.sdf");

  const auto other = build_sdf_plan(320, 180, L"cache:morph.b");
  MB_CHECK(other.has_value());
  MB_CHECK(plan->seed != other->seed);
  MB_CHECK(plan->final_sdf != other->final_sdf);
  MB_CHECK(!build_sdf_plan(0, 180, L"cache:morph.a"));
  MB_CHECK(!build_sdf_plan(320, 180, L""));
}

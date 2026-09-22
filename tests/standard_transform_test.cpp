#include "render/standard_transform.hpp"
#include "test_support.hpp"

using morph_bridge::StandardTransform;
using morph_bridge::TransformCorrection;
using morph_bridge::interpolate_angle_degrees;
using morph_bridge::interpolate_corrected_transform;
using morph_bridge::interpolate_positive_scale;

void run_standard_transform_tests() {
  StandardTransform a{};
  a.x = 10.0;
  a.rx = 350.0;
  a.scale = 50.0;
  a.opacity = 20.0;
  StandardTransform b{};
  b.x = 110.0;
  b.rx = 10.0;
  b.scale = 200.0;
  b.opacity = 80.0;

  TransformCorrection a_fix{};
  a_fix.x = 20.0;
  a_fix.scale = 10.0;
  TransformCorrection b_fix{};
  b_fix.x = -10.0;
  b_fix.scale = -20.0;

  const auto at_a = interpolate_corrected_transform(a, a_fix, b, b_fix, 0.0);
  MB_CHECK_NEAR(at_a.x, a.x + a_fix.x, 0.0001);
  MB_CHECK_NEAR(at_a.scale, a.scale + a_fix.scale, 0.0001);
  const auto at_b = interpolate_corrected_transform(a, a_fix, b, b_fix, 1.0);
  MB_CHECK_NEAR(at_b.x, b.x + b_fix.x, 0.0001);
  MB_CHECK_NEAR(at_b.scale, b.scale + b_fix.scale, 0.0001);

  MB_CHECK_NEAR(interpolate_angle_degrees(350.0, 10.0, 0.5), 0.0, 0.0001);
  MB_CHECK_NEAR(interpolate_positive_scale(50.0, 200.0, 0.5), 100.0, 0.001);

  const auto middle = interpolate_corrected_transform(a, a_fix, b, b_fix, 0.5);
  MB_CHECK_NEAR(middle.x, 65.0, 0.0001);
  MB_CHECK_NEAR(middle.rx, 0.0, 0.0001);
  MB_CHECK_NEAR(middle.opacity, 50.0, 0.0001);

  const auto before_start = interpolate_corrected_transform(a, a_fix, b, b_fix, -1.0);
  const auto after_end = interpolate_corrected_transform(a, a_fix, b, b_fix, 2.0);
  MB_CHECK_NEAR(before_start.x, at_a.x, 0.0001);
  MB_CHECK_NEAR(after_end.x, at_b.x, 0.0001);
}

#include "render/standard_transform.hpp"
#include "test_support.hpp"

using morph_bridge::StandardTransform;
using morph_bridge::TransformCorrection;
using morph_bridge::interpolate_angle_degrees;
using morph_bridge::interpolate_transform;
using morph_bridge::make_sampling_transform;
using morph_bridge::make_sampling_transforms;

void run_standard_transform_tests() {
  StandardTransform a{};
  a.x = 10.0;
  a.rx = 350.0;
  a.scale = 50.0;
  a.aspect = 20.0;
  a.cx = 10.0;
  a.cy = -5.0;
  a.depth_scale = 0.5;
  a.opacity = 20.0;
  StandardTransform b{};
  b.x = 110.0;
  b.rx = 10.0;
  b.scale = 200.0;
  b.depth_scale = 2.0;
  b.opacity = 80.0;

  const auto at_a = interpolate_transform(a, b, 0.0);
  MB_CHECK_NEAR(at_a.x, a.x, 0.0001);
  MB_CHECK_NEAR(at_a.scale, 100.0, 0.0001);
  MB_CHECK_NEAR(at_a.aspect, 0.0, 0.0001);
  MB_CHECK_NEAR(at_a.depth_scale, 0.5, 0.0001);
  const auto at_b = interpolate_transform(a, b, 1.0);
  MB_CHECK_NEAR(at_b.x, b.x, 0.0001);
  MB_CHECK_NEAR(at_b.scale, 100.0, 0.0001);
  MB_CHECK_NEAR(at_b.aspect, 0.0, 0.0001);
  MB_CHECK_NEAR(at_b.depth_scale, 2.0, 0.0001);

  MB_CHECK_NEAR(interpolate_angle_degrees(350.0, 10.0, 0.5), 0.0, 0.0001);
  const auto middle = interpolate_transform(a, b, 0.5);
  MB_CHECK_NEAR(middle.x, 60.0, 0.0001);
  MB_CHECK_NEAR(middle.rx, 0.0, 0.0001);
  MB_CHECK_NEAR(middle.opacity, 50.0, 0.0001);
  MB_CHECK_NEAR(middle.depth_scale, 1.0, 0.0001);

  const auto before_start = interpolate_transform(a, b, -1.0);
  const auto after_end = interpolate_transform(a, b, 2.0);
  MB_CHECK_NEAR(before_start.x, -90.0, 0.0001);
  MB_CHECK_NEAR(after_end.x, 210.0, 0.0001);
  MB_CHECK_NEAR(before_start.rx, 330.0, 0.0001);
  MB_CHECK_NEAR(after_end.rx, 30.0, 0.0001);
  MB_CHECK_NEAR(before_start.scale, 100.0, 0.0001);
  MB_CHECK_NEAR(after_end.scale, 100.0, 0.0001);

  TransformCorrection correction{};
  correction.x = 10.0;
  correction.y = -20.0;
  correction.rotation = 30.0;
  correction.scale = 200.0;
  correction.aspect = 10.0;
  const auto sampling = make_sampling_transform(correction, 0.5, 100.0F, 50.0F);
  MB_CHECK_NEAR(sampling.tx, 5.0F, 0.0001F);
  MB_CHECK_NEAR(sampling.ty, -10.0F, 0.0001F);
  MB_CHECK_NEAR(sampling.rotation, 15.0F, 0.0001F);
  MB_CHECK_NEAR(sampling.sx, 1.575F, 0.0001F);
  MB_CHECK_NEAR(sampling.sy, 1.425F, 0.0001F);
  MB_CHECK_NEAR(sampling.cx, 100.0F, 0.0001F);
  MB_CHECK_NEAR(sampling.cy, 50.0F, 0.0001F);

  const auto extrapolated = make_sampling_transform(correction, 2.0, 100.0F, 50.0F);
  MB_CHECK_NEAR(extrapolated.tx, 20.0F, 0.0001F);
  MB_CHECK_NEAR(extrapolated.rotation, 60.0F, 0.0001F);
  MB_CHECK_NEAR(extrapolated.sx, 3.6F, 0.0001F);
  MB_CHECK_NEAR(extrapolated.sy, 2.4F, 0.0001F);

  const auto identity_correction = make_sampling_transform(
      TransformCorrection{}, 1.0, 100.0F, 50.0F);
  MB_CHECK_NEAR(identity_correction.sx, 1.0F, 0.0001F);
  MB_CHECK_NEAR(identity_correction.sy, 1.0F, 0.0001F);

  const auto source_scaled = make_sampling_transform(
      a, correction, 0.5, 100.0F, 50.0F);
  MB_CHECK_NEAR(source_scaled.sx, 0.945F, 0.0001F);
  MB_CHECK_NEAR(source_scaled.sy, 0.57F, 0.0001F);

  const auto source_pivoted = make_sampling_transform(
      a, TransformCorrection{}, 0.0, 100.0F, 50.0F);
  MB_CHECK_NEAR(source_pivoted.tx, 4.0F, 0.0001F);
  MB_CHECK_NEAR(source_pivoted.ty, -3.0F, 0.0001F);

  TransformCorrection after_correction{};
  after_correction.x = 20.0;
  const auto pair = make_sampling_transforms(
      correction, after_correction, 1.25, 100.0F, 50.0F, 80.0F, 40.0F);
  MB_CHECK_NEAR(pair.first.tx, -2.5F, 0.0001F);
  MB_CHECK_NEAR(pair.second.tx, 25.0F, 0.0001F);
}

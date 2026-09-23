#include "render/standard_transform.hpp"

#include <algorithm>
#include <cmath>

namespace morph_bridge {
namespace {

constexpr double minimum_factor = 1.0e-6;

double lerp(const double before, const double after, const double progress) {
  return before + (after - before) * progress;
}

double wrap_degrees(double angle) {
  angle = std::fmod(angle, 360.0);
  if (angle < 0.0) {
    angle += 360.0;
  }
  if (std::abs(angle - 360.0) < 1.0e-9 || std::abs(angle) < 1.0e-9) {
    return 0.0;
  }
  return angle;
}

}  // namespace

double interpolate_angle_degrees(
    const double before, const double after, const double progress) {
  double delta = std::fmod(after - before + 540.0, 360.0) - 180.0;
  if (delta < -180.0) {
    delta += 360.0;
  }
  return wrap_degrees(before + delta * progress);
}

ScaleFactors scale_factors(
    const double scale_percent, const double aspect_percent) {
  const double scale = std::max(scale_percent / 100.0, minimum_factor);
  const double aspect = std::clamp(aspect_percent / 100.0, -0.999999, 0.999999);
  return {
      std::max(scale * (1.0 + aspect), minimum_factor),
      std::max(scale * (1.0 - aspect), minimum_factor)};
}

StandardTransform interpolate_transform(
    const StandardTransform& before,
    const StandardTransform& after,
    const double progress) {
  StandardTransform result;
  result.x = lerp(before.x, after.x, progress);
  result.y = lerp(before.y, after.y, progress);
  result.z = lerp(before.z, after.z, progress);
  result.cx = lerp(before.cx, after.cx, progress);
  result.cy = lerp(before.cy, after.cy, progress);
  result.cz = lerp(before.cz, after.cz, progress);
  result.rx = interpolate_angle_degrees(before.rx, after.rx, progress);
  result.ry = interpolate_angle_degrees(before.ry, after.ry, progress);
  result.rz = interpolate_angle_degrees(before.rz, after.rz, progress);
  const double before_depth = std::max(std::abs(before.depth_scale), minimum_factor);
  const double after_depth = std::max(std::abs(after.depth_scale), minimum_factor);
  result.depth_scale = std::exp(
      lerp(std::log(before_depth), std::log(after_depth), progress));
  result.opacity = lerp(before.opacity, after.opacity, progress);
  return result;
}

SamplingTransform make_sampling_transform(
    const TransformCorrection& correction,
    const double weight,
    const float center_x,
    const float center_y) {
  const double scale = lerp(1.0, correction.scale / 100.0, weight);
  const double aspect = correction.aspect * weight / 100.0;
  SamplingTransform result;
  result.tx = static_cast<float>(correction.x * weight);
  result.ty = static_cast<float>(correction.y * weight);
  result.rotation = static_cast<float>(correction.rotation * weight);
  result.sx = static_cast<float>(scale * (1.0 + aspect));
  result.sy = static_cast<float>(scale * (1.0 - aspect));
  result.cx = center_x;
  result.cy = center_y;
  return result;
}

SamplingTransform make_sampling_transform(
    const StandardTransform& source,
    const TransformCorrection& correction,
    const double weight,
    const float center_x,
    const float center_y) {
  auto result = make_sampling_transform(correction, weight, center_x, center_y);
  const auto source_scale = scale_factors(source.scale, source.aspect);
  const double pivot_x = (1.0 - source_scale.x) * source.cx;
  const double pivot_y = (1.0 - source_scale.y) * source.cy;
  constexpr double pi = 3.14159265358979323846;
  const double radians = result.rotation * pi / 180.0;
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
  result.tx += static_cast<float>(
      cosine * result.sx * pivot_x - sine * result.sy * pivot_y);
  result.ty += static_cast<float>(
      sine * result.sx * pivot_x + cosine * result.sy * pivot_y);
  result.sx *= static_cast<float>(source_scale.x);
  result.sy *= static_cast<float>(source_scale.y);
  return result;
}

std::pair<SamplingTransform, SamplingTransform> make_sampling_transforms(
    const TransformCorrection& before,
    const TransformCorrection& after,
    const double progress,
    const float before_center_x,
    const float before_center_y,
    const float after_center_x,
    const float after_center_y) {
  return {
      make_sampling_transform(
          before, 1.0 - progress, before_center_x, before_center_y),
      make_sampling_transform(
          after, progress, after_center_x, after_center_y)};
}

}  // namespace morph_bridge

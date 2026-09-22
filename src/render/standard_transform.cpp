#include "render/standard_transform.hpp"

#include <algorithm>
#include <cmath>

namespace morph_bridge {
namespace {

constexpr double minimum_factor = 1.0e-6;

double lerp(const double before, const double after, const double progress) {
  return before + (after - before) * progress;
}

StandardTransform corrected(
    const StandardTransform& value, const TransformCorrection& correction) {
  auto result = value;
  result.x += correction.x;
  result.y += correction.y;
  result.z += correction.z;
  result.cx += correction.cx;
  result.cy += correction.cy;
  result.cz += correction.cz;
  result.rx += correction.rx;
  result.ry += correction.ry;
  result.rz += correction.rz;
  result.scale += correction.scale;
  result.aspect += correction.aspect;
  return result;
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
  const double amount = std::clamp(progress, 0.0, 1.0);
  double delta = std::fmod(after - before + 540.0, 360.0) - 180.0;
  if (delta < -180.0) {
    delta += 360.0;
  }
  return wrap_degrees(before + delta * amount);
}

double interpolate_positive_scale(
    const double before, const double after, const double progress) {
  const double amount = std::clamp(progress, 0.0, 1.0);
  const double safe_before = std::max(before, minimum_factor);
  const double safe_after = std::max(after, minimum_factor);
  return std::exp(lerp(std::log(safe_before), std::log(safe_after), amount));
}

ScaleFactors scale_factors(
    const double scale_percent, const double aspect_percent) {
  const double scale = std::max(scale_percent / 100.0, minimum_factor);
  const double aspect = std::clamp(aspect_percent / 100.0, -0.999999, 0.999999);
  return {
      std::max(scale * (1.0 + aspect), minimum_factor),
      std::max(scale * (1.0 - aspect), minimum_factor)};
}

StandardTransform interpolate_corrected_transform(
    const StandardTransform& before,
    const TransformCorrection& before_correction,
    const StandardTransform& after,
    const TransformCorrection& after_correction,
    const double progress) {
  const double amount = std::clamp(progress, 0.0, 1.0);
  const auto a = corrected(before, before_correction);
  const auto b = corrected(after, after_correction);
  const auto a_factors = scale_factors(a.scale, a.aspect);
  const auto b_factors = scale_factors(b.scale, b.aspect);
  const double sx = interpolate_positive_scale(a_factors.x, b_factors.x, amount);
  const double sy = interpolate_positive_scale(a_factors.y, b_factors.y, amount);
  const double factor_sum = sx + sy;

  StandardTransform result;
  result.x = lerp(a.x, b.x, amount);
  result.y = lerp(a.y, b.y, amount);
  result.z = lerp(a.z, b.z, amount);
  result.cx = lerp(a.cx, b.cx, amount);
  result.cy = lerp(a.cy, b.cy, amount);
  result.cz = lerp(a.cz, b.cz, amount);
  result.rx = interpolate_angle_degrees(a.rx, b.rx, amount);
  result.ry = interpolate_angle_degrees(a.ry, b.ry, amount);
  result.rz = interpolate_angle_degrees(a.rz, b.rz, amount);
  result.scale = factor_sum * 50.0;
  result.aspect = factor_sum > 0.0 ? (sx - sy) / factor_sum * 100.0 : 0.0;
  result.opacity = lerp(a.opacity, b.opacity, amount);
  return result;
}

}  // namespace morph_bridge

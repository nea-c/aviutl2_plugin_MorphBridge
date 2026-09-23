#ifdef __cplusplus
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace morph_bridge {
inline constexpr std::uint16_t sdf_invalid_coordinate = 0xffffU;
inline constexpr std::uint32_t sdf_encoding_version = 2U;
inline float extend_sdf_distance(
    const float border_distance, const float offset_x, const float offset_y) {
  return border_distance + std::hypot(offset_x, offset_y);
}
inline float signed_sdf_distance_from_seed(
    const float unsigned_distance,
    const bool is_inside,
    const float maximum_distance) {
  const float magnitude =
      std::min(unsigned_distance + 0.5F, maximum_distance);
  return is_inside ? -magnitude : magnitude;
}
inline float threshold_crossing_fraction(
    const float center_alpha,
    const float neighbor_alpha,
    const float threshold) {
  const float delta = neighbor_alpha - center_alpha;
  if (std::abs(delta) <= 1.0e-6F) return 0.5F;
  return std::clamp((threshold - center_alpha) / delta, 0.0F, 1.0F);
}
inline bool mask_sample_inside(
    const float alpha,
    const float threshold,
    const bool in_bounds) {
  return in_bounds && alpha >= threshold;
}
inline float signed_sdf_distance_to_contour(
    const float offset_x,
    const float offset_y,
    const bool is_inside,
    const float maximum_distance) {
  const float magnitude =
      std::min(std::hypot(offset_x, offset_y), maximum_distance);
  return is_inside ? -magnitude : magnitude;
}
}  // namespace morph_bridge
#else
#define MB_SDF_INVALID_COORDINATE 65535
#define MB_SDF_ENCODING_VERSION 2
float ExtendSdfDistance(
    const float border_distance, const float offset_x, const float offset_y) {
  return border_distance + length(float2(offset_x, offset_y));
}
float SignedSdfDistanceFromSeed(
    const float unsigned_distance,
    const bool is_inside,
    const float maximum_distance) {
  const float magnitude = min(unsigned_distance + 0.5, maximum_distance);
  return is_inside ? -magnitude : magnitude;
}
float ThresholdCrossingFraction(
    const float center_alpha,
    const float neighbor_alpha,
    const float threshold) {
  const float delta = neighbor_alpha - center_alpha;
  if (abs(delta) <= 1.0e-6) return 0.5;
  return saturate((threshold - center_alpha) / delta);
}
bool MaskSampleInside(
    const float alpha,
    const float threshold,
    const bool in_bounds) {
  return in_bounds && alpha >= threshold;
}
float SignedSdfDistanceToContour(
    const float2 offset,
    const bool is_inside,
    const float maximum_distance) {
  const float magnitude = min(length(offset), maximum_distance);
  return is_inside ? -magnitude : magnitude;
}
#endif

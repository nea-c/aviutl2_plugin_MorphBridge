#ifdef __cplusplus
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace morph_bridge {
inline constexpr std::uint16_t sdf_invalid_coordinate = 0xffffU;
inline constexpr std::uint32_t sdf_encoding_version = 4U;
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
inline float distance_to_contour_segment(
    const float point_x,
    const float point_y,
    const float start_x,
    const float start_y,
    const float end_x,
    const float end_y) {
  const float segment_x = end_x - start_x;
  const float segment_y = end_y - start_y;
  const float length_squared =
      segment_x * segment_x + segment_y * segment_y;
  const float amount = length_squared > 1.0e-12F
      ? std::clamp(
            ((point_x - start_x) * segment_x +
             (point_y - start_y) * segment_y) /
                length_squared,
            0.0F, 1.0F)
      : 0.0F;
  return std::hypot(
      point_x - (start_x + segment_x * amount),
      point_y - (start_y + segment_y * amount));
}
inline bool marching_squares_pair_first_adjacent(
    const bool corner_zero_inside,
    const bool center_inside) {
  return corner_zero_inside == center_inside;
}
inline bool marching_squares_cell_has_contour(
    const bool corner_zero_inside,
    const bool corner_one_inside,
    const bool corner_two_inside,
    const bool corner_three_inside) {
  return corner_zero_inside != corner_one_inside ||
      corner_one_inside != corner_two_inside ||
      corner_two_inside != corner_three_inside ||
      corner_three_inside != corner_zero_inside;
}
}  // namespace morph_bridge
#else
#define MB_SDF_INVALID_COORDINATE 65535
#define MB_SDF_ENCODING_VERSION 4
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
float DistanceToContourSegment(
    const float2 sample_position,
    const float2 segment_start,
    const float2 segment_end) {
  const float2 segment = segment_end - segment_start;
  const float length_squared = dot(segment, segment);
  const float amount = length_squared > 1.0e-12
      ? saturate(dot(sample_position - segment_start, segment) / length_squared)
      : 0.0;
  return length(
      sample_position - (segment_start + segment * amount));
}
bool MarchingSquaresPairFirstAdjacent(
    const bool corner_zero_inside,
    const bool center_inside) {
  return corner_zero_inside == center_inside;
}
bool MarchingSquaresCellHasContour(
    const bool corner_zero_inside,
    const bool corner_one_inside,
    const bool corner_two_inside,
    const bool corner_three_inside) {
  return corner_zero_inside != corner_one_inside ||
      corner_one_inside != corner_two_inside ||
      corner_two_inside != corner_three_inside ||
      corner_three_inside != corner_zero_inside;
}
#endif

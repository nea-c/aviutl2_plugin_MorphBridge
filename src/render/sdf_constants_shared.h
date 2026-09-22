#ifdef __cplusplus
#pragma once
#include <cmath>
#include <cstdint>
namespace morph_bridge {
inline constexpr std::uint16_t sdf_invalid_coordinate = 0xffffU;
inline constexpr std::uint32_t sdf_encoding_version = 1U;
inline float extend_sdf_distance(
    const float border_distance, const float offset_x, const float offset_y) {
  return border_distance + std::hypot(offset_x, offset_y);
}
}  // namespace morph_bridge
#else
#define MB_SDF_INVALID_COORDINATE 65535
#define MB_SDF_ENCODING_VERSION 1
float ExtendSdfDistance(
    const float border_distance, const float offset_x, const float offset_y) {
  return border_distance + length(float2(offset_x, offset_y));
}
#endif

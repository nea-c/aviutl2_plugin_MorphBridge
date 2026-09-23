#include "render/sdf_encoding.hpp"
#include "render/sdf_coverage_shared.h"
#include "render/sdf_constants_shared.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace morph_bridge {
namespace {

std::uint16_t unpack_u16(const std::uint8_t low, const std::uint8_t high) {
  return static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(low) |
      static_cast<std::uint16_t>(static_cast<std::uint16_t>(high) << 8U));
}

}  // namespace

PackedRgba8 invalid_seed() {
  return {0xffU, 0xffU, 0xffU, 0xffU};
}

PackedRgba8 pack_seed(const std::uint16_t x, const std::uint16_t y) {
  if (x == sdf_invalid_coordinate || y == sdf_invalid_coordinate) {
    return invalid_seed();
  }
  return {
      static_cast<std::uint8_t>(x & 0xffU),
      static_cast<std::uint8_t>(x >> 8U),
      static_cast<std::uint8_t>(y & 0xffU),
      static_cast<std::uint8_t>(y >> 8U)};
}

std::optional<SeedCoordinate> unpack_seed(const PackedRgba8 packed) {
  const auto x = unpack_u16(packed[0], packed[1]);
  const auto y = unpack_u16(packed[2], packed[3]);
  if (x == sdf_invalid_coordinate || y == sdf_invalid_coordinate) {
    return std::nullopt;
  }
  return SeedCoordinate{x, y};
}

PackedRgba8 pack_signed_distance(const float distance, const float maximum_distance) {
  if (!(maximum_distance > 0.0F) || !std::isfinite(maximum_distance)) {
    return {0U, 0U, 0U, 1U};
  }
  const float normalized = std::clamp(
      distance / maximum_distance * 0.5F + 0.5F, 0.0F, 1.0F);
  const auto encoded = static_cast<std::uint16_t>(
      std::lround(normalized * 65535.0F));
  return {
      static_cast<std::uint8_t>(encoded & 0xffU),
      static_cast<std::uint8_t>(encoded >> 8U),
      static_cast<std::uint8_t>(distance <= 0.0F ? 0xffU : 0U),
      1U};
}

float unpack_signed_distance(const PackedRgba8 packed, const float maximum_distance) {
  const auto encoded = unpack_u16(packed[0], packed[1]);
  const float normalized = static_cast<float>(encoded) / 65535.0F;
  return (normalized * 2.0F - 1.0F) * maximum_distance;
}

float interpolate_sdf(const float before, const float after, const float progress) {
  return before + (after - before) * progress;
}

float stabilize_extrapolated_sdf(
    const float before,
    const float after,
    const float progress,
    const float distance_limit) {
  float distance = interpolate_sdf(before, after, progress);
  if (progress < 0.0F) {
    distance = std::max(distance, before - distance_limit);
  } else if (progress > 1.0F) {
    distance = std::max(distance, after - distance_limit);
  }
  return distance;
}

bool inside(const float distance) {
  return distance <= 0.0F;
}

float alpha_from_sdf(const float distance, const float feather_width) {
  return SdfAntialiasCoverage(distance, feather_width);
}

}  // namespace morph_bridge

#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace morph_bridge {

using PackedRgba8 = std::array<std::uint8_t, 4>;

struct SeedCoordinate {
  std::uint16_t x{};
  std::uint16_t y{};
  friend bool operator==(const SeedCoordinate&, const SeedCoordinate&) = default;
};

[[nodiscard]] PackedRgba8 invalid_seed();
[[nodiscard]] PackedRgba8 pack_seed(std::uint16_t x, std::uint16_t y);
[[nodiscard]] std::optional<SeedCoordinate> unpack_seed(PackedRgba8 packed);

[[nodiscard]] PackedRgba8 pack_signed_distance(float distance, float maximum_distance);
[[nodiscard]] float unpack_signed_distance(PackedRgba8 packed, float maximum_distance);
[[nodiscard]] float interpolate_sdf(float before, float after, float progress);
[[nodiscard]] bool inside(float distance);
[[nodiscard]] float alpha_from_sdf(float distance, float feather_width);

}  // namespace morph_bridge

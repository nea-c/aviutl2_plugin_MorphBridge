#include "render/sdf_encoding.hpp"
#include "test_support.hpp"

#include <array>
#include <cmath>
#include <cstdint>

using morph_bridge::SeedCoordinate;
using morph_bridge::alpha_from_sdf;
using morph_bridge::inside;
using morph_bridge::interpolate_sdf;
using morph_bridge::pack_seed;
using morph_bridge::pack_signed_distance;
using morph_bridge::unpack_seed;
using morph_bridge::unpack_signed_distance;

void run_sdf_encoding_tests() {
  for (const std::uint16_t value : std::array<std::uint16_t, 5>{0, 1, 255, 256, 4095}) {
    const auto decoded = unpack_seed(pack_seed(value, value));
    MB_CHECK(decoded.has_value());
    const SeedCoordinate expected{value, value};
    MB_CHECK(*decoded == expected);
  }
  MB_CHECK(!unpack_seed(morph_bridge::invalid_seed()));

  constexpr float max_distance = 64.0F;
  constexpr float unit = (2.0F * max_distance) / 65535.0F;
  for (const float value : std::array<float, 3>{-max_distance, 0.0F, max_distance}) {
    MB_CHECK_NEAR(unpack_signed_distance(pack_signed_distance(value, max_distance), max_distance),
                  value, unit);
  }

  MB_CHECK(inside(interpolate_sdf(-10.0F, 10.0F, 0.25F)));
  MB_CHECK(!inside(interpolate_sdf(-10.0F, 10.0F, 0.75F)));
  MB_CHECK(alpha_from_sdf(0.0F, 1.0F) > 0.49F);
}

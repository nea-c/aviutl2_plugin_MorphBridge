#include "render/canvas.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace morph_bridge {
namespace {

constexpr int maximum_sdf_dimension = 65'534;

int scaled_dimension(const int value, const int scale) {
  return static_cast<int>((static_cast<std::int64_t>(value) * scale + 99) / 100);
}

int make_even(const int value) {
  return value + (value & 1);
}

}  // namespace

std::optional<CanvasLayout> make_canvas_layout(
    const int before_width,
    const int before_height,
    const int after_width,
    const int after_height,
    const int scale_percent,
    const int margin) {
  if (before_width <= 0 || before_height <= 0 || after_width <= 0 ||
      after_height <= 0 || margin < 0 ||
      (scale_percent != 25 && scale_percent != 50 && scale_percent != 100)) {
    return std::nullopt;
  }

  const int before_scaled_width = scaled_dimension(before_width, scale_percent);
  const int before_scaled_height = scaled_dimension(before_height, scale_percent);
  const int after_scaled_width = scaled_dimension(after_width, scale_percent);
  const int after_scaled_height = scaled_dimension(after_height, scale_percent);
  const int content_width = std::max(before_scaled_width, after_scaled_width);
  const int content_height = std::max(before_scaled_height, after_scaled_height);
  const auto requested_width = static_cast<std::int64_t>(content_width) + 2LL * margin;
  const auto requested_height = static_cast<std::int64_t>(content_height) + 2LL * margin;
  if (requested_width <= 0 || requested_height <= 0 ||
      requested_width > maximum_sdf_dimension || requested_height > maximum_sdf_dimension) {
    return std::nullopt;
  }

  const int canvas_width = make_even(static_cast<int>(requested_width));
  const int canvas_height = make_even(static_cast<int>(requested_height));
  if (canvas_width > maximum_sdf_dimension || canvas_height > maximum_sdf_dimension) {
    return std::nullopt;
  }

  const auto place = [=](const int width, const int height) {
    return PlacementRect{
        margin + (content_width - width) / 2,
        margin + (content_height - height) / 2,
        width,
        height};
  };
  return CanvasLayout{
      canvas_width,
      canvas_height,
      place(before_scaled_width, before_scaled_height),
      place(after_scaled_width, after_scaled_height),
      SamplingTransform{},
      SamplingTransform{}};
}

}  // namespace morph_bridge

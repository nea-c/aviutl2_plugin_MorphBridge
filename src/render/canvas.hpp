#pragma once

#include "domain/types.hpp"

#include <optional>

namespace morph_bridge {

struct PlacementRect {
  int left{};
  int top{};
  int width{};
  int height{};
  friend bool operator==(const PlacementRect&, const PlacementRect&) = default;
};

struct CanvasLayout {
  int width{};
  int height{};
  PlacementRect before;
  PlacementRect after;
  SamplingTransform before_sampling;
  SamplingTransform after_sampling;
};

[[nodiscard]] std::optional<CanvasLayout> make_canvas_layout(
    int before_width,
    int before_height,
    int after_width,
    int after_height,
    int scale_percent,
    int margin);

}  // namespace morph_bridge

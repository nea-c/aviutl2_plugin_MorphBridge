#include "render/canvas.hpp"

#include <algorithm>
#include <cmath>
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

struct Bounds {
  double left{};
  double top{};
  double right{};
  double bottom{};
};

Bounds transformed_bounds(
    const PlacementRect& rect, const SamplingTransform& transform) {
  constexpr double pi = 3.14159265358979323846;
  const double radians = static_cast<double>(transform.rotation) * pi / 180.0;
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
  const double half_width = rect.width * 0.5;
  const double half_height = rect.height * 0.5;
  const double extent_x =
      std::abs(cosine * transform.sx) * half_width +
      std::abs(sine * transform.sy) * half_height;
  const double extent_y =
      std::abs(sine * transform.sx) * half_width +
      std::abs(cosine * transform.sy) * half_height;
  const double center_x = rect.left + half_width + transform.tx;
  const double center_y = rect.top + half_height + transform.ty;
  return {
      center_x - extent_x,
      center_y - extent_y,
      center_x + extent_x,
      center_y + extent_y};
}

bool finite_transform(const SamplingTransform& transform) {
  return std::isfinite(transform.tx) && std::isfinite(transform.ty) &&
         std::isfinite(transform.rotation) && std::isfinite(transform.sx) &&
         std::isfinite(transform.sy);
}

}  // namespace

std::optional<ExtrapolationEnvelope> make_extrapolation_envelope(
    const int before_width,
    const int before_height,
    const int after_width,
    const int after_height,
    const int scale_percent) {
  if (before_width <= 0 || before_height <= 0 || after_width <= 0 ||
      after_height <= 0 ||
      (scale_percent != 25 && scale_percent != 50 && scale_percent != 100)) {
    return std::nullopt;
  }
  const int maximum_content_dimension = std::max({
      scaled_dimension(before_width, scale_percent),
      scaled_dimension(before_height, scale_percent),
      scaled_dimension(after_width, scale_percent),
      scaled_dimension(after_height, scale_percent)});
  const int distance_limit = std::clamp(
      (maximum_content_dimension + 3) / 4, 16, 128);
  return ExtrapolationEnvelope{distance_limit + 4, distance_limit};
}

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

std::optional<CanvasLayout> make_transformed_canvas_layout(
    const int before_width,
    const int before_height,
    const int after_width,
    const int after_height,
    const int scale_percent,
    const int base_margin,
    SamplingTransform before_sampling,
    SamplingTransform after_sampling) {
  if (base_margin < 0 || !finite_transform(before_sampling) ||
      !finite_transform(after_sampling)) {
    return std::nullopt;
  }
  const auto content = make_canvas_layout(
      before_width, before_height, after_width, after_height, scale_percent, 0);
  if (!content) {
    return std::nullopt;
  }

  const auto before_bounds = transformed_bounds(content->before, before_sampling);
  const auto after_bounds = transformed_bounds(content->after, after_sampling);
  const double overflow = std::max({
      0.0,
      -before_bounds.left,
      -before_bounds.top,
      before_bounds.right - content->width,
      before_bounds.bottom - content->height,
      -after_bounds.left,
      -after_bounds.top,
      after_bounds.right - content->width,
      after_bounds.bottom - content->height});
  if (!std::isfinite(overflow) ||
      overflow > static_cast<double>(maximum_sdf_dimension)) {
    return std::nullopt;
  }
  const auto extra_margin = static_cast<std::int64_t>(std::ceil(overflow));
  const auto requested_margin = static_cast<std::int64_t>(base_margin) + extra_margin;
  if (requested_margin > std::numeric_limits<int>::max()) {
    return std::nullopt;
  }
  auto result = make_canvas_layout(
      before_width, before_height, after_width, after_height, scale_percent,
      static_cast<int>(requested_margin));
  if (!result) {
    return std::nullopt;
  }
  before_sampling.cx = result->before.left + result->before.width * 0.5F;
  before_sampling.cy = result->before.top + result->before.height * 0.5F;
  after_sampling.cx = result->after.left + result->after.width * 0.5F;
  after_sampling.cy = result->after.top + result->after.height * 0.5F;
  result->before_sampling = before_sampling;
  result->after_sampling = after_sampling;
  return result;
}

}  // namespace morph_bridge

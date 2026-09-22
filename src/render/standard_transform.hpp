#pragma once

#include "domain/types.hpp"

#include <utility>

namespace morph_bridge {

struct StandardTransform {
  double x{};
  double y{};
  double z{};
  double cx{};
  double cy{};
  double cz{};
  double rx{};
  double ry{};
  double rz{};
  double scale{100.0};
  double aspect{};
  double opacity{100.0};
};

struct TransformCorrection {
  double x{};
  double y{};
  double scale{100.0};
  double aspect{};
  double rotation{};
};

struct ScaleFactors {
  double x{1.0};
  double y{1.0};
};

[[nodiscard]] double interpolate_angle_degrees(double before, double after, double progress);
[[nodiscard]] double interpolate_positive_scale(double before, double after, double progress);
[[nodiscard]] ScaleFactors scale_factors(double scale_percent, double aspect_percent);
[[nodiscard]] StandardTransform interpolate_transform(
    const StandardTransform& before, const StandardTransform& after, double progress);
[[nodiscard]] SamplingTransform make_sampling_transform(
    const TransformCorrection& correction,
    double weight,
    float center_x,
    float center_y);
[[nodiscard]] std::pair<SamplingTransform, SamplingTransform> make_sampling_transforms(
    const TransformCorrection& before,
    const TransformCorrection& after,
    double progress,
    float before_center_x,
    float before_center_y,
    float after_center_x,
    float after_center_y);

}  // namespace morph_bridge

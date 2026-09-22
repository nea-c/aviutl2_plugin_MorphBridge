#pragma once

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
  double z{};
  double cx{};
  double cy{};
  double cz{};
  double rx{};
  double ry{};
  double rz{};
  double scale{};
  double aspect{};
};

struct ScaleFactors {
  double x{1.0};
  double y{1.0};
};

[[nodiscard]] double interpolate_angle_degrees(double before, double after, double progress);
[[nodiscard]] double interpolate_positive_scale(double before, double after, double progress);
[[nodiscard]] ScaleFactors scale_factors(double scale_percent, double aspect_percent);
[[nodiscard]] StandardTransform interpolate_corrected_transform(
    const StandardTransform& before,
    const TransformCorrection& before_correction,
    const StandardTransform& after,
    const TransformCorrection& after_correction,
    double progress);

}  // namespace morph_bridge

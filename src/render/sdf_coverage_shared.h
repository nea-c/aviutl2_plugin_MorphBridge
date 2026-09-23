#ifndef MORPH_BRIDGE_SDF_COVERAGE_SHARED_H
#define MORPH_BRIDGE_SDF_COVERAGE_SHARED_H

#ifdef __cplusplus
#include <algorithm>
#include <cmath>

static inline float SdfCoverageSaturate(const float value) {
  return std::clamp(value, 0.0F, 1.0F);
}
static inline float SdfCoverageAbs(const float value) { return std::abs(value); }
static inline float SdfCoverageSin(const float value) { return std::sin(value); }
static inline float SdfCoverageAsin(const float value) { return std::asin(value); }
#else
static float SdfCoverageSaturate(const float value) {
  return saturate(value);
}
static float SdfCoverageAbs(const float value) { return abs(value); }
static float SdfCoverageSin(const float value) { return sin(value); }
static float SdfCoverageAsin(const float value) { return asin(value); }
#endif

static inline float SdfPixelCoordinate(const float transformed_coordinate) {
  // AviUtl2 places the rendered media image half a pixel before the captured
  // resource phase. Sampling half a source pixel earlier cancels that offset.
  return transformed_coordinate - 1.0F;
}

static inline float SdfAntialiasCoverage(
    const float distance,
    const float transition_width) {
  if (!(transition_width > 0.0F)) {
    return distance <= 0.0F ? 1.0F : 0.0F;
  }
  const float amount = SdfCoverageSaturate(
      distance / transition_width + 0.5F);
  const float smooth_amount = amount * amount * (3.0F - 2.0F * amount);
  return 1.0F - smooth_amount;
}

static inline float SdfInverseSmoothstep(const float value) {
  const float amount = SdfCoverageSaturate(value);
  return 0.5F -
      SdfCoverageSin(SdfCoverageAsin(1.0F - 2.0F * amount) / 3.0F);
}

static inline float SdfDistanceFromCoverage(
    const float geometric_distance,
    const float coverage,
    const float transition_width,
    const float refinement_band) {
  if (!(transition_width > 0.0F) || !(refinement_band > 0.0F) ||
      SdfCoverageAbs(geometric_distance) > refinement_band ||
      coverage <= 0.0F || coverage >= 1.0F) {
    return geometric_distance;
  }
  const float amount = SdfInverseSmoothstep(1.0F - coverage);
  return (amount - 0.5F) * transition_width;
}

#endif

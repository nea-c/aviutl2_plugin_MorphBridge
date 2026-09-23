#ifndef MORPH_BRIDGE_SDF_COVERAGE_SHARED_H
#define MORPH_BRIDGE_SDF_COVERAGE_SHARED_H

#ifdef __cplusplus
#include <algorithm>

static inline float SdfCoverageSaturate(const float value) {
  return std::clamp(value, 0.0F, 1.0F);
}
#else
static float SdfCoverageSaturate(const float value) {
  return saturate(value);
}
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

#endif

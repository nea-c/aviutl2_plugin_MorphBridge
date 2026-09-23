#ifndef MORPH_BRIDGE_ALPHA_BLEND_SHARED_H
#define MORPH_BRIDGE_ALPHA_BLEND_SHARED_H

#ifdef __cplusplus
#include <algorithm>

static inline float MorphSaturate(const float value) {
  return std::clamp(value, 0.0F, 1.0F);
}
#else
static float MorphSaturate(const float value) {
  return saturate(value);
}
#endif

static inline float MorphPreservedAlpha(
    const float before_alpha,
    const float after_alpha,
    const float before_coverage,
    const float after_coverage,
    const float morph_coverage,
    const float progress) {
  if (progress == 0.0F) {
    return MorphSaturate(before_alpha);
  }
  if (progress == 1.0F) {
    return MorphSaturate(after_alpha);
  }
  if (progress < 0.0F) {
    if (before_coverage <= 1.0e-6F) {
      return 0.0F;
    }
    return MorphSaturate(
        morph_coverage * before_alpha / before_coverage);
  }
  if (progress > 1.0F) {
    if (after_coverage <= 1.0e-6F) {
      return 0.0F;
    }
    return MorphSaturate(
        morph_coverage * after_alpha / after_coverage);
  }
  const float amount = MorphSaturate(progress);
  const float weighted_coverage =
      (1.0F - amount) * before_coverage + amount * after_coverage;
  if (weighted_coverage <= 1.0e-6F) {
    return 0.0F;
  }
  const float weighted_alpha =
      (1.0F - amount) * before_alpha + amount * after_alpha;
  return MorphSaturate(morph_coverage * weighted_alpha / weighted_coverage);
}

#endif

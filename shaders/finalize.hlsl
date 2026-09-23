#include "render/sdf_constants_shared.h"

Texture2D<float4> Seeds : register(t0);
Texture2D<float4> Mask : register(t1);
RWTexture2D<float4> Target : register(u0);

cbuffer FinalizeConstants : register(b0) {
  uint Width;
  uint Height;
  float AlphaThreshold;
  float MaximumDistance;
};

uint2 DecodeSeed(float4 packed) {
  const uint4 bytes = uint4(round(saturate(packed) * 255.0));
  return uint2(bytes.x | (bytes.y << 8u), bytes.z | (bytes.w << 8u));
}

bool ValidSeed(uint2 seed) {
  return seed.x != MB_SDF_INVALID_COORDINATE &&
      seed.y != MB_SDF_INVALID_COORDINATE;
}

bool MaskInBounds(int2 pixel) {
  return pixel.x >= 0 && pixel.y >= 0 &&
      pixel.x < int(Width) && pixel.y < int(Height);
}

float LoadMaskAlpha(int2 pixel) {
  if (!MaskInBounds(pixel)) {
    return 0.0;
  }
  return Mask.Load(int3(pixel, 0)).a;
}

void ConsiderSeedContour(
    uint2 pixel,
    uint2 seed,
    float threshold,
    inout float best_distance,
    inout float2 best_offset,
    inout bool found_crossing) {
  const int2 offsets[4] = {
      int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1)};
  const float center_alpha = LoadMaskAlpha(int2(seed));
  const bool center_inside = MaskSampleInside(
      center_alpha, threshold, true);
  [unroll]
  for (int index = 0; index < 4; ++index) {
    const int2 neighbor = int2(seed) + offsets[index];
    const float neighbor_alpha = LoadMaskAlpha(neighbor);
    const bool neighbor_inside = MaskSampleInside(
        neighbor_alpha, threshold, MaskInBounds(neighbor));
    if (neighbor_inside != center_inside) {
      const float amount = ThresholdCrossingFraction(
          center_alpha, neighbor_alpha, threshold);
      const float2 contour = float2(seed) + float2(offsets[index]) * amount;
      const float2 contour_offset = contour - float2(pixel);
      const float candidate_distance = length(contour_offset);
      if (candidate_distance < best_distance) {
        best_distance = candidate_distance;
        best_offset = contour_offset;
      }
      found_crossing = true;
    }
  }
}

float DistanceToNearestContour(
    uint2 pixel, float threshold, bool is_inside) {
  uint2 candidates[9];
  [unroll]
  for (int clear_index = 0; clear_index < 9; ++clear_index) {
    candidates[clear_index] = uint2(
        MB_SDF_INVALID_COORDINATE, MB_SDF_INVALID_COORDINATE);
  }
  [unroll]
  for (int y = -1; y <= 1; ++y) {
    [unroll]
    for (int x = -1; x <= 1; ++x) {
      const int slot = (y + 1) * 3 + (x + 1);
      const int2 candidate_pixel = int2(pixel) + int2(x, y);
      if (candidate_pixel.x < 0 || candidate_pixel.y < 0 ||
          candidate_pixel.x >= int(Width) ||
          candidate_pixel.y >= int(Height)) continue;
      const uint2 candidate = DecodeSeed(
          Seeds.Load(int3(candidate_pixel, 0)));
      if (ValidSeed(candidate)) candidates[slot] = candidate;
    }
  }

  float best_distance = MaximumDistance;
  float2 best_offset = float2(MaximumDistance, 0.0);
  bool found_crossing = false;
  uint2 fallback_seed = uint2(
      MB_SDF_INVALID_COORDINATE, MB_SDF_INVALID_COORDINATE);
  float fallback_distance = MaximumDistance;
  [unroll]
  for (int candidate_index = 0; candidate_index < 9; ++candidate_index) {
    if (!ValidSeed(candidates[candidate_index])) continue;
    const float candidate_seed_distance = length(
        float2(candidates[candidate_index]) - float2(pixel));
    if (candidate_seed_distance < fallback_distance) {
      fallback_distance = candidate_seed_distance;
      fallback_seed = candidates[candidate_index];
    }
    bool duplicate = false;
    [unroll]
    for (int previous = 0; previous < 9; ++previous) {
      if (previous < candidate_index && ValidSeed(candidates[previous]) &&
          all(candidates[previous] == candidates[candidate_index])) {
        duplicate = true;
      }
    }
    if (duplicate) continue;
    ConsiderSeedContour(
        pixel, candidates[candidate_index], threshold,
        best_distance, best_offset, found_crossing);
  }
  if (!found_crossing) {
    if (!ValidSeed(fallback_seed)) {
      return is_inside ? -MaximumDistance : MaximumDistance;
    }
    return SignedSdfDistanceFromSeed(
        length(float2(fallback_seed) - float2(pixel)),
        is_inside, MaximumDistance);
  }
  return SignedSdfDistanceToContour(
      best_offset, is_inside, MaximumDistance);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_id.xy;
  if (pixel.x >= Width || pixel.y >= Height) return;
  const bool is_inside =
      Mask.Load(int3(pixel, 0)).a >= AlphaThreshold;
  const float distance = DistanceToNearestContour(
      pixel, AlphaThreshold, is_inside);
  const uint encoded = uint(round(saturate(distance / MaximumDistance * 0.5 + 0.5) * 65535.0));
  Target[pixel] = float4(
      (encoded & 255u) / 255.0,
      ((encoded >> 8u) & 255u) / 255.0,
      distance <= 0.0 ? 1.0 : 0.0,
      MB_SDF_ENCODING_VERSION / 255.0);
}

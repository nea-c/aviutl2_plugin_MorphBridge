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

void ConsiderSegment(
    uint2 pixel,
    float2 start,
    float2 end,
    inout float best_distance,
    inout bool found_crossing) {
  const float candidate_distance = DistanceToContourSegment(
      float2(pixel), start, end);
  if (candidate_distance < best_distance) {
    best_distance = candidate_distance;
  }
  found_crossing = true;
}

void ConsiderCellContour(
    uint2 pixel,
    int2 cell,
    float threshold,
    inout float best_distance,
    inout bool found_crossing) {
  const int2 corners[4] = {
      cell,
      cell + int2(1, 0),
      cell + int2(1, 1),
      cell + int2(0, 1)};
  float alpha[4];
  bool inside[4];
  [unroll]
  for (int corner = 0; corner < 4; ++corner) {
    alpha[corner] = LoadMaskAlpha(corners[corner]);
    inside[corner] = MaskSampleInside(
        alpha[corner], threshold, MaskInBounds(corners[corner]));
  }

  float2 crossings[4];
  int crossing_count = 0;
  [unroll]
  for (int edge = 0; edge < 4; ++edge) {
    const int next = (edge + 1) & 3;
    if (inside[edge] == inside[next]) continue;
    const float amount = ThresholdCrossingFraction(
        alpha[edge], alpha[next], threshold);
    crossings[crossing_count] =
        lerp(float2(corners[edge]), float2(corners[next]), amount);
    ++crossing_count;
  }

  if (crossing_count == 2) {
    ConsiderSegment(
        pixel, crossings[0], crossings[1],
        best_distance, found_crossing);
  } else if (crossing_count == 4) {
    const bool center_inside =
        (alpha[0] + alpha[1] + alpha[2] + alpha[3]) * 0.25 >= threshold;
    if (MarchingSquaresPairFirstAdjacent(inside[0], center_inside)) {
      ConsiderSegment(
          pixel, crossings[0], crossings[1],
          best_distance, found_crossing);
      ConsiderSegment(
          pixel, crossings[2], crossings[3],
          best_distance, found_crossing);
    } else {
      ConsiderSegment(
          pixel, crossings[3], crossings[0],
          best_distance, found_crossing);
      ConsiderSegment(
          pixel, crossings[1], crossings[2],
          best_distance, found_crossing);
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
    ConsiderCellContour(
        pixel, int2(candidates[candidate_index]), threshold,
        best_distance, found_crossing);
  }
  if (!found_crossing) {
    if (!ValidSeed(fallback_seed)) {
      return is_inside ? -MaximumDistance : MaximumDistance;
    }
    return SignedSdfDistanceFromSeed(
        length(float2(fallback_seed) - float2(pixel)),
        is_inside, MaximumDistance);
  }
  const float magnitude = min(best_distance, MaximumDistance);
  return is_inside ? -magnitude : magnitude;
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

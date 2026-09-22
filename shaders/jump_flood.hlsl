#include "render/sdf_constants_shared.h"

Texture2D<float4> Source : register(t0);
RWTexture2D<float4> Target : register(u0);

cbuffer JumpConstants : register(b0) {
  uint Width;
  uint Height;
  uint Step;
  uint Padding;
};

uint2 DecodeSeed(float4 packed) {
  const uint4 bytes = uint4(round(saturate(packed) * 255.0));
  return uint2(bytes.x | (bytes.y << 8u), bytes.z | (bytes.w << 8u));
}

bool ValidSeed(uint2 seed) {
  return seed.x != MB_SDF_INVALID_COORDINATE && seed.y != MB_SDF_INVALID_COORDINATE;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_id.xy;
  if (pixel.x >= Width || pixel.y >= Height) return;
  float4 best_packed = Source.Load(int3(pixel, 0));
  uint2 best_seed = DecodeSeed(best_packed);
  float best_distance = ValidSeed(best_seed)
      ? dot(float2(best_seed) - float2(pixel), float2(best_seed) - float2(pixel))
      : 3.402823466e+38F;
  [unroll]
  for (int y = -1; y <= 1; ++y) {
    [unroll]
    for (int x = -1; x <= 1; ++x) {
      const int2 candidate_pixel = int2(pixel) + int2(x, y) * int(Step);
      if (candidate_pixel.x < 0 || candidate_pixel.y < 0 ||
          candidate_pixel.x >= int(Width) || candidate_pixel.y >= int(Height)) continue;
      const float4 packed = Source.Load(int3(candidate_pixel, 0));
      const uint2 seed = DecodeSeed(packed);
      if (!ValidSeed(seed)) continue;
      const float2 delta = float2(seed) - float2(pixel);
      const float distance = dot(delta, delta);
      if (distance < best_distance) {
        best_distance = distance;
        best_seed = seed;
        best_packed = packed;
      }
    }
  }
  Target[pixel] = best_packed;
}

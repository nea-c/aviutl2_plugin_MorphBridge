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

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_id.xy;
  if (pixel.x >= Width || pixel.y >= Height) return;
  const uint2 seed = DecodeSeed(Seeds.Load(int3(pixel, 0)));
  float distance = MaximumDistance;
  if (seed.x != MB_SDF_INVALID_COORDINATE && seed.y != MB_SDF_INVALID_COORDINATE) {
    distance = min(length(float2(seed) - float2(pixel)), MaximumDistance);
  }
  if (Mask.Load(int3(pixel, 0)).a >= AlphaThreshold) distance = -distance;
  const uint encoded = uint(round(saturate(distance / MaximumDistance * 0.5 + 0.5) * 65535.0));
  Target[pixel] = float4(
      (encoded & 255u) / 255.0,
      ((encoded >> 8u) & 255u) / 255.0,
      distance <= 0.0 ? 1.0 : 0.0,
      MB_SDF_ENCODING_VERSION / 255.0);
}

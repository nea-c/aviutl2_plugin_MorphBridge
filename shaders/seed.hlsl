#include "render/sdf_constants_shared.h"

Texture2D<float4> Source : register(t0);
RWTexture2D<float4> Target : register(u0);

cbuffer SeedConstants : register(b0) {
  uint Width;
  uint Height;
  float AlphaThreshold;
  uint Padding;
};

float4 EncodeSeed(uint2 coordinate) {
  return float4(
      (coordinate.x & 255u) / 255.0,
      ((coordinate.x >> 8u) & 255u) / 255.0,
      (coordinate.y & 255u) / 255.0,
      ((coordinate.y >> 8u) & 255u) / 255.0);
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_id.xy;
  if (pixel.x >= Width || pixel.y >= Height) return;
  const bool center_inside = Source.Load(int3(pixel, 0)).a >= AlphaThreshold;
  bool boundary = false;
  const int2 offsets[4] = {int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1)};
  [unroll]
  for (int index = 0; index < 4; ++index) {
    const int2 neighbor = int2(pixel) + offsets[index];
    const bool neighbor_inside = neighbor.x >= 0 && neighbor.y >= 0 &&
        neighbor.x < int(Width) && neighbor.y < int(Height)
        ? Source.Load(int3(neighbor, 0)).a >= AlphaThreshold
        : false;
    boundary = boundary || (neighbor_inside != center_inside);
  }
  Target[pixel] = boundary ? EncodeSeed(pixel) : 1.0.xxxx;
}

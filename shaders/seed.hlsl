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

bool SourceInBounds(int2 pixel) {
  return pixel.x >= 0 && pixel.y >= 0 &&
      pixel.x < int(Width) && pixel.y < int(Height);
}

float SourceAlpha(int2 pixel) {
  return SourceInBounds(pixel)
      ? Source.Load(int3(pixel, 0)).a
      : 0.0;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
  const uint2 pixel = dispatch_id.xy;
  if (pixel.x >= Width || pixel.y >= Height) return;
  const int2 corners[4] = {
      int2(pixel),
      int2(pixel) + int2(1, 0),
      int2(pixel) + int2(1, 1),
      int2(pixel) + int2(0, 1)};
  bool inside[4];
  [unroll]
  for (int corner = 0; corner < 4; ++corner) {
    inside[corner] = SourceInBounds(corners[corner]) &&
        SourceAlpha(corners[corner]) >= AlphaThreshold;
  }
  const bool boundary = MarchingSquaresCellHasContour(
      inside[0], inside[1], inside[2], inside[3]);
  Target[pixel] = boundary ? EncodeSeed(pixel) : 1.0.xxxx;
}

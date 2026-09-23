#include "render/sdf_constants_shared.h"
#include "render/sdf_coverage_shared.h"

Texture2D<float4> BeforeSdf : register(t0);
Texture2D<float4> AfterSdf : register(t1);

cbuffer MorphConstants : register(b0) {
  float4 SolidColor;
  float Progress;
  float MaximumDistance;
  float FeatherWidth;
  float OutputOpacity;
  float ExtrapolationLimit;
  float3 MorphPadding;
  float4 BeforeInverseRow0;
  float4 BeforeInverseRow1;
  float4 AfterInverseRow0;
  float4 AfterInverseRow1;
};

float DecodeDistance(float4 packed) {
  const uint2 bytes = uint2(round(saturate(packed.rg) * 255.0));
  const uint encoded = bytes.x | (bytes.y << 8u);
  return (encoded / 65535.0 * 2.0 - 1.0) * MaximumDistance;
}

float2 TransformPosition(float2 sample_position, float4 row0, float4 row1) {
  return float2(
      dot(float3(sample_position, 1.0), row0.xyz),
      dot(float3(sample_position, 1.0), row1.xyz));
}

float LoadDistance(Texture2D<float4> field, int2 pixel, uint width, uint height) {
  const int2 clamped = clamp(pixel, int2(0, 0), int2(width - 1u, height - 1u));
  return DecodeDistance(field.Load(int3(clamped, 0)));
}

float SampleDistance(Texture2D<float4> field, float2 transformed) {
  uint width;
  uint height;
  field.GetDimensions(width, height);
  const float2 pixel_position = transformed - 0.5;
  const float2 clamped_position = clamp(
      pixel_position, float2(0.0, 0.0), float2(width - 1u, height - 1u));
  const int2 base = int2(floor(clamped_position));
  const float2 fraction = frac(clamped_position);
  const float top = lerp(
      LoadDistance(field, base, width, height),
      LoadDistance(field, base + int2(1, 0), width, height), fraction.x);
  const float bottom = lerp(
      LoadDistance(field, base + int2(0, 1), width, height),
      LoadDistance(field, base + int2(1, 1), width, height), fraction.x);
  const float border_distance = lerp(top, bottom, fraction.y);
  const float2 outside_offset = pixel_position - clamped_position;
  return ExtendSdfDistance(
      border_distance, outside_offset.x, outside_offset.y);
}

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
  const float2 before_position = TransformPosition(
      position.xy, BeforeInverseRow0, BeforeInverseRow1);
  const float2 after_position = TransformPosition(
      position.xy, AfterInverseRow0, AfterInverseRow1);
  const float before = SampleDistance(BeforeSdf, before_position);
  const float after = SampleDistance(AfterSdf, after_position);
  float distance = lerp(before, after, Progress);
  if (Progress < 0.0) {
    distance = max(distance, before - ExtrapolationLimit);
  } else if (Progress > 1.0) {
    distance = max(distance, after - ExtrapolationLimit);
  }
  const float transition_width = max(
      fwidth(distance) * FeatherWidth, 0.0001);
  const float alpha =
      SdfAntialiasCoverage(distance, transition_width) * OutputOpacity;
  return float4(SolidColor.rgb * alpha, alpha);
}

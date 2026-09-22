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

float SampleDistance(Texture2D<float4> field, float2 sample_position, float4 row0, float4 row1) {
  const float2 transformed = float2(
      dot(float3(sample_position, 1.0), row0.xyz),
      dot(float3(sample_position, 1.0), row1.xyz));
  uint width;
  uint height;
  field.GetDimensions(width, height);
  if (transformed.x < 0.0 || transformed.y < 0.0 ||
      transformed.x >= float(width) || transformed.y >= float(height)) return MaximumDistance;
  return DecodeDistance(field.Load(int3(int2(transformed), 0)));
}

float4 main(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target {
  const float before = SampleDistance(BeforeSdf, position.xy, BeforeInverseRow0, BeforeInverseRow1);
  const float after = SampleDistance(AfterSdf, position.xy, AfterInverseRow0, AfterInverseRow1);
  float distance = lerp(before, after, Progress);
  if (Progress < 0.0) {
    distance = max(distance, before - ExtrapolationLimit);
  } else if (Progress > 1.0) {
    distance = max(distance, after - ExtrapolationLimit);
  }
  const float alpha = saturate(0.5 - distance / max(FeatherWidth, 0.0001)) * OutputOpacity;
  return float4(SolidColor.rgb * alpha, alpha);
}

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace morph_bridge {

struct ObjectSpan {
  std::int64_t token{};
  int layer{};
  int start{};
  int end{};
  friend bool operator==(const ObjectSpan&, const ObjectSpan&) = default;
};

struct EndpointPair {
  ObjectSpan before;
  ObjectSpan after;
  int before_frame{};
  int after_frame{};
  friend bool operator==(const EndpointPair&, const EndpointPair&) = default;
};

struct RgbaImage {
  int width{};
  int height{};
  std::vector<std::byte> pixels;
};

struct SamplingTransform {
  float tx{};
  float ty{};
  float rotation{};
  float sx{1.0F};
  float sy{1.0F};
  friend bool operator==(const SamplingTransform&, const SamplingTransform&) = default;
};

enum class CacheStatus { Empty, Dirty, Capturing, CpuReady, GpuReady, Error };

}  // namespace morph_bridge

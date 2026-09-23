#pragma once

#include "domain/types.hpp"

#include <cstdint>
#include <string>

namespace morph_bridge {

struct EndpointDescriptor {
  ObjectSpan span;
  int frame{};
  std::string alias_utf8;
  friend bool operator==(const EndpointDescriptor&, const EndpointDescriptor&) = default;
};

struct EndpointSignature {
  std::uint64_t high{};
  std::uint64_t low{};
  friend bool operator==(const EndpointSignature&, const EndpointSignature&) = default;
};

[[nodiscard]] EndpointSignature make_signature(
    std::int64_t scene_token,
    int scene_width,
    int scene_height,
    const EndpointDescriptor& before,
    const EndpointDescriptor& after,
    int sdf_scale_percent,
    std::uint32_t cache_format_version);

}  // namespace morph_bridge

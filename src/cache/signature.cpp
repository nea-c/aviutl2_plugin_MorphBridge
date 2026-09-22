#include "cache/signature.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace morph_bridge {
namespace {

class Fnv1a64 {
 public:
  explicit Fnv1a64(const std::uint64_t offset) : value_(offset) {}

  template <typename Integer>
    requires std::is_integral_v<Integer>
  void integer(const Integer input) {
    using Unsigned = std::make_unsigned_t<Integer>;
    auto value = static_cast<Unsigned>(input);
    for (std::size_t index = 0; index < sizeof(value); ++index) {
      byte(static_cast<std::uint8_t>(value & static_cast<Unsigned>(0xffU)));
      value >>= 8U;
    }
  }

  void text(const std::string_view input) {
    integer(static_cast<std::uint64_t>(input.size()));
    for (const unsigned char value : input) {
      byte(value);
    }
  }

  [[nodiscard]] std::uint64_t value() const { return value_; }

 private:
  void byte(const std::uint8_t value) {
    value_ ^= value;
    value_ *= 1'099'511'628'211ULL;
  }

  std::uint64_t value_;
};

void add_span(Fnv1a64& hash, const ObjectSpan& span) {
  hash.integer(span.token);
  hash.integer(span.layer);
  hash.integer(span.start);
  hash.integer(span.end);
}

void add_endpoint(Fnv1a64& hash, const EndpointDescriptor& endpoint) {
  add_span(hash, endpoint.span);
  hash.integer(endpoint.frame);
  hash.text(endpoint.alias_utf8);
}

void add_all(
    Fnv1a64& hash,
    const std::int64_t scene_token,
    const int scene_width,
    const int scene_height,
    const EndpointDescriptor& before,
    const EndpointDescriptor& after,
    const int alpha_threshold_basis_points,
    const int sdf_scale_percent,
    const std::uint32_t cache_format_version) {
  hash.integer(cache_format_version);
  hash.integer(scene_token);
  hash.integer(scene_width);
  hash.integer(scene_height);
  add_endpoint(hash, before);
  add_endpoint(hash, after);
  hash.integer(alpha_threshold_basis_points);
  hash.integer(sdf_scale_percent);
}

}  // namespace

int quantize_alpha_threshold(const double percent) {
  return static_cast<int>(std::lround(std::clamp(percent, 0.0, 100.0) * 100.0));
}

EndpointSignature make_signature(
    const std::int64_t scene_token,
    const int scene_width,
    const int scene_height,
    const EndpointDescriptor& before,
    const EndpointDescriptor& after,
    const int alpha_threshold_basis_points,
    const int sdf_scale_percent,
    const std::uint32_t cache_format_version) {
  Fnv1a64 high{14'695'981'039'346'656'037ULL};
  Fnv1a64 low{7'807'829'856'337'127'661ULL};
  add_all(high, scene_token, scene_width, scene_height, before, after,
          alpha_threshold_basis_points, sdf_scale_percent, cache_format_version);
  add_all(low, scene_token, scene_width, scene_height, before, after,
          alpha_threshold_basis_points, sdf_scale_percent, cache_format_version);
  return {high.value(), low.value()};
}

}  // namespace morph_bridge

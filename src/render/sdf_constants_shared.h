#ifdef __cplusplus
#pragma once
#include <cstdint>
namespace morph_bridge {
inline constexpr std::uint16_t sdf_invalid_coordinate = 0xffffU;
inline constexpr std::uint32_t sdf_encoding_version = 1U;
}  // namespace morph_bridge
#else
#define MB_SDF_INVALID_COORDINATE 65535
#define MB_SDF_ENCODING_VERSION 1
#endif

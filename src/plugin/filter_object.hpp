#pragma once

#include <cstdint>
#include <windows.h>

#include "plugin2.h"
#include "filter2.h"

namespace morph_bridge {

[[nodiscard]] FILTER_PLUGIN_TABLE* filter_plugin_table();
[[nodiscard]] bool register_morph_bridge(HOST_APP_TABLE* host);
[[nodiscard]] std::uint32_t required_version();

}  // namespace morph_bridge

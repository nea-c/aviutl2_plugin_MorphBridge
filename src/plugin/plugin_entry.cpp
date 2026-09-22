#include <windows.h>
#include <cstdint>

#include "plugin2.h"
#include "filter2.h"
#include "plugin/filter_object.hpp"

namespace {

COMMON_PLUGIN_TABLE plugin_table{
    L"MorphBridge",
    L"SDF silhouette morph between adjacent objects",
};

}  // namespace

extern "C" __declspec(dllexport) COMMON_PLUGIN_TABLE* GetCommonPluginTable() {
  return &plugin_table;
}

extern "C" __declspec(dllexport) bool InitializePlugin(DWORD) { return true; }

extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE* host) {
  (void)morph_bridge::register_morph_bridge(host);
}

extern "C" __declspec(dllexport) void UninitializePlugin() {}

extern "C" __declspec(dllexport) DWORD RequiredVersion() {
  return morph_bridge::required_version();
}

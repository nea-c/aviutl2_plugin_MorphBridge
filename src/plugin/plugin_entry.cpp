#include <windows.h>
#include <cstdint>

#include "plugin2.h"
#include "filter2.h"

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

extern "C" __declspec(dllexport) void RegisterPlugin(HOST_APP_TABLE*) {}

extern "C" __declspec(dllexport) void UninitializePlugin() {}

extern "C" __declspec(dllexport) DWORD RequiredVersion() { return 2'010'800; }

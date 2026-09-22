#include "plugin/filter_object.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

FILTER_PLUGIN_TABLE* registered_filter{};
EDIT_HANDLE fake_edit_handle{};
bool created_edit_handle{};
std::vector<EVENT_TYPE> registered_events;
bool clear_handler_registered{};

struct FilterItemPrefix {
  LPCWSTR type;
  LPCWSTR name;
};

void register_filter(FILTER_PLUGIN_TABLE* table) { registered_filter = table; }

EDIT_HANDLE* create_edit() {
  created_edit_handle = true;
  return &fake_edit_handle;
}

void register_event(EVENT_TYPE type, void*, void (*)(void*)) {
  registered_events.push_back(type);
}

void register_clear(void (*handler)(EDIT_SECTION*)) {
  clear_handler_registered = handler != nullptr;
}

}  // namespace

void run_plugin_registration_tests() {
  registered_filter = nullptr;
  created_edit_handle = false;
  registered_events.clear();
  clear_handler_registered = false;

  HOST_APP_TABLE host{};
  host.register_filter_plugin = &register_filter;
  host.create_edit_handle = &create_edit;
  host.register_event_listener = &register_event;
  host.register_clear_cache_handler = &register_clear;

  MB_CHECK(morph_bridge::register_morph_bridge(&host));
  MB_CHECK(registered_filter != nullptr);
  MB_CHECK((registered_filter->flag & FILTER_PLUGIN_TABLE::FLAG_VIDEO) != 0);
  MB_CHECK((registered_filter->flag & FILTER_PLUGIN_TABLE::FLAG_INPUT) != 0);
  MB_CHECK((registered_filter->flag & FILTER_PLUGIN_TABLE::FLAG_USERDATA) != 0);
  MB_CHECK(created_edit_handle);
  MB_CHECK(std::find(registered_events.begin(), registered_events.end(),
                     EVENT_TYPE::UPDATE_OBJECT) != registered_events.end());
  MB_CHECK(std::find(registered_events.begin(), registered_events.end(),
                     EVENT_TYPE::CHANGE_EDIT_SCENE) != registered_events.end());
  MB_CHECK(clear_handler_registered);
  MB_CHECK(morph_bridge::required_version() == 2'010'800);
  MB_CHECK(registered_filter->items != nullptr);
  auto* progress = static_cast<FILTER_ITEM_TRACK*>(registered_filter->items[0]);
  MB_CHECK(std::wstring{progress->name} == L"進捗");
  MB_CHECK(progress->value == 0.0);
  int item_count = 0;
  bool has_before_corrections = false;
  bool has_after_corrections = false;
  std::set<std::wstring> value_item_names;
  std::map<std::wstring, const FILTER_ITEM_TRACK*> tracks;
  for (void** item = registered_filter->items; *item != nullptr; ++item) {
    const auto* prefix = static_cast<const FilterItemPrefix*>(*item);
    MB_CHECK(std::wstring{prefix->type} != L"trackgroup");
    if (std::wstring{prefix->type} == L"group") {
      has_before_corrections |= std::wstring{prefix->name} == L"前オブジェクト補正";
      has_after_corrections |= std::wstring{prefix->name} == L"後オブジェクト補正";
    } else {
      MB_CHECK(value_item_names.insert(prefix->name).second);
      if (std::wstring{prefix->type} == L"track2") {
        const auto* track = static_cast<const FILTER_ITEM_TRACK*>(*item);
        tracks.emplace(track->name, track);
      }
    }
    ++item_count;
  }
  MB_CHECK(has_before_corrections);
  MB_CHECK(has_after_corrections);
  MB_CHECK(value_item_names.contains(L"色"));
  MB_CHECK(value_item_names.contains(L"しきい値"));
  MB_CHECK(value_item_names.contains(L"前オブジェクト補正::X"));
  MB_CHECK(value_item_names.contains(L"前オブジェクト補正::Y"));
  MB_CHECK(value_item_names.contains(L"前オブジェクト補正::拡大率"));
  MB_CHECK(value_item_names.contains(L"前オブジェクト補正::回転"));
  MB_CHECK(value_item_names.contains(L"前オブジェクト補正::縦横比"));
  MB_CHECK(value_item_names.contains(L"後オブジェクト補正::X"));
  MB_CHECK(value_item_names.contains(L"後オブジェクト補正::Y"));
  MB_CHECK(value_item_names.contains(L"後オブジェクト補正::拡大率"));
  MB_CHECK(value_item_names.contains(L"後オブジェクト補正::回転"));
  MB_CHECK(value_item_names.contains(L"後オブジェクト補正::縦横比"));
  const auto check_track = [&](const wchar_t* name, const double value,
                               const double minimum, const double maximum,
                               const double step) {
    const auto* track = tracks.at(name);
    MB_CHECK(track->value == value);
    MB_CHECK(track->s == minimum);
    MB_CHECK(track->e == maximum);
    MB_CHECK(track->step == step);
  };
  check_track(L"しきい値", 50.0, 0.0, 100.0, 0.01);
  check_track(L"前オブジェクト補正::X", 0.0, -100000.0, 100000.0, 0.01);
  check_track(L"前オブジェクト補正::Y", 0.0, -100000.0, 100000.0, 0.01);
  check_track(L"前オブジェクト補正::拡大率", 100.0, 0.0, 10000.0, 0.001);
  check_track(L"前オブジェクト補正::回転", 0.0, -3600.0, 3600.0, 0.01);
  check_track(L"前オブジェクト補正::縦横比", 0.0, -100.0, 100.0, 0.001);
  check_track(L"後オブジェクト補正::X", 0.0, -100000.0, 100000.0, 0.01);
  check_track(L"後オブジェクト補正::Y", 0.0, -100000.0, 100000.0, 0.01);
  check_track(L"後オブジェクト補正::拡大率", 100.0, 0.0, 10000.0, 0.001);
  check_track(L"後オブジェクト補正::回転", 0.0, -3600.0, 3600.0, 0.01);
  check_track(L"後オブジェクト補正::縦横比", 0.0, -100.0, 100.0, 0.001);
  MB_CHECK(!value_item_names.contains(L"SDF scale"));
  MB_CHECK(item_count == 17);
  MB_CHECK(registered_filter->func_create != nullptr);
  MB_CHECK(registered_filter->func_destroy != nullptr);
}

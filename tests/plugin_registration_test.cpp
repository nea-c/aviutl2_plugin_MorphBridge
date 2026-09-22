#include "plugin/filter_object.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace {

FILTER_PLUGIN_TABLE* registered_filter{};
EDIT_HANDLE fake_edit_handle{};
bool created_edit_handle{};
std::vector<EVENT_TYPE> registered_events;
bool clear_handler_registered{};

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
  MB_CHECK(std::wstring{progress->name} == L"Progress");
  MB_CHECK(progress->value == 0.0);
  int item_count = 0;
  for (void** item = registered_filter->items; *item != nullptr; ++item) {
    ++item_count;
  }
  MB_CHECK(item_count == 12);
  MB_CHECK(registered_filter->func_create != nullptr);
  MB_CHECK(registered_filter->func_destroy != nullptr);
}

#include "plugin/filter_object.hpp"
#include "test_support.hpp"

#include <array>
#include <cwchar>

namespace {

int output_width{};
int output_height{};
int capture_request_count{};

struct FakeObject {
  OBJECT_LAYER_FRAME span;
  bool morph_bridge{};
};

std::array<FakeObject, 4> split_objects{{
    {{2, 0, 9}, false},
    {{2, 10, 14}, true},
    {{2, 15, 19}, true},
    {{2, 20, 29}, false},
}};
EDIT_HANDLE split_render_handle{};

void set_image(const PIXEL_RGBA*, int width, int height) {
  output_width = width;
  output_height = height;
}

OBJECT_HANDLE find_split_object(int layer, int frame) {
  FakeObject* next{};
  for (auto& object : split_objects) {
    if (object.span.layer != layer) continue;
    if (frame >= object.span.start && frame <= object.span.end) {
      return &object;
    }
    if (object.span.start > frame &&
        (next == nullptr || object.span.start < next->span.start)) {
      next = &object;
    }
  }
  return next;
}

OBJECT_LAYER_FRAME get_split_span(OBJECT_HANDLE object) {
  return static_cast<FakeObject*>(object)->span;
}

int count_split_effect(OBJECT_HANDLE object, LPCWSTR effect) {
  return static_cast<FakeObject*>(object)->morph_bridge &&
                 std::wcscmp(effect, L"MorphBridge") == 0
             ? 1
             : 0;
}

LPCSTR get_split_alias(OBJECT_HANDLE object) {
  return static_cast<FakeObject*>(object)->morph_bridge
             ? "[Object]\neffect.name=MorphBridge\n"
             : "[Object]\neffect.name=Shape\n";
}

bool queue_split_capture(
    OBJECT_HANDLE, int, bool, void*,
    void (*)(void*, int, const void*, int, int, int)) {
  ++capture_request_count;
  return false;
}

bool get_split_transform(
    OBJECT_HANDLE, double, OBJECT_IMAGE_PARAM* param, int size) {
  if (param == nullptr || size != sizeof(*param)) return false;
  *param = {};
  param->sx = 1.0F;
  param->sy = 1.0F;
  param->sz = 1.0F;
  param->alpha = 1.0F;
  return true;
}

void ignore_filter_registration(FILTER_PLUGIN_TABLE*) {}
EDIT_HANDLE* create_split_edit_handle() { return &split_render_handle; }
void ignore_event_registration(EVENT_TYPE, void*, void (*)(void*)) {}
void ignore_clear_registration(void (*)(EDIT_SECTION*)) {}

void run_split_neighbor_test() {
  split_render_handle = {};
  split_render_handle.rendering_object_video = &queue_split_capture;
  HOST_APP_TABLE host{};
  host.register_filter_plugin = &ignore_filter_registration;
  host.create_edit_handle = &create_split_edit_handle;
  host.register_event_listener = &ignore_event_registration;
  host.register_clear_cache_handler = &ignore_clear_registration;
  MB_CHECK(morph_bridge::register_morph_bridge(&host));

  EDIT_INFO edit_info{};
  edit_info.scene_id = 1;
  EDIT_SECTION edit{};
  edit.info = &edit_info;
  edit.find_object = &find_split_object;
  edit.count_object_effect = &count_split_effect;
  edit.get_object_layer_frame = &get_split_span;
  edit.get_object_alias = &get_split_alias;

  SCENE_INFO scene{};
  scene.width = 1920;
  scene.height = 1080;
  scene.rate = 30;
  scene.scale = 1;
  auto* table = morph_bridge::filter_plugin_table();
  const auto run = [&](const std::int64_t effect_id, const int frame_s,
                       const int frame_e, const int expected_captures) {
    OBJECT_INFO object{};
    object.effect_id = effect_id;
    object.layer = 2;
    object.frame_s = frame_s;
    object.frame_e = frame_e;
    OBJECT_IMAGE_PARAM output_param{};
    void* state = table->func_create(object.effect_id);
    MB_CHECK(state != nullptr);
    FILTER_PROC_VIDEO video{};
    video.scene = &scene;
    video.object = &object;
    video.edit = &edit;
    video.param = &output_param;
    video.userdata = state;
    video.set_image_data = &set_image;
    video.get_output_image_param = &get_split_transform;
    capture_request_count = 0;
    output_width = 0;
    output_height = 0;

    MB_CHECK(table->func_proc_video(&video));
    MB_CHECK(capture_request_count == expected_captures);
    MB_CHECK(output_width == 1);
    MB_CHECK(output_height == 1);
    table->func_destroy(object.effect_id, state);
  };

  run(456, 10, 14, 0);
  run(457, 15, 19, 0);

  split_objects[2].morph_bridge = false;
  run(458, 10, 14, 1);
  split_objects[2].morph_bridge = true;
}

}  // namespace

void run_filter_object_tests() {
  auto* table = morph_bridge::filter_plugin_table();
  MB_CHECK(table != nullptr);
  void* state = table->func_create(123);
  MB_CHECK(state != nullptr);

  OBJECT_INFO object{};
  object.effect_id = 123;
  object.layer = 2;
  object.frame_s = 10;
  object.frame_e = 19;
  FILTER_PROC_VIDEO video{};
  video.object = &object;
  video.userdata = state;
  video.set_image_data = &set_image;
  output_width = 0;
  output_height = 0;
  MB_CHECK(table->func_proc_video(&video));
  MB_CHECK(output_width == 1);
  MB_CHECK(output_height == 1);

  table->func_destroy(123, state);

  run_split_neighbor_test();
}

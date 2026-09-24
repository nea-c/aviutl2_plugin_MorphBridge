#include "plugin/filter_object.hpp"
#include "test_support.hpp"

#include <array>
#include <cwchar>

namespace {

int output_width{};
int output_height{};
int capture_request_count{};
float before_sx{1.0F};
float before_sy{1.0F};
float after_sx{1.0F};
float after_sy{1.0F};
std::array<float, 4> before_inverse_row{};
std::array<float, 4> after_inverse_row{};
int pixel_shader_count{};

struct MorphConstantsProbe {
  std::array<float, 4> color;
  float progress;
  float maximum_distance;
  float feather_width;
  float opacity;
  float extrapolation_limit;
  std::array<float, 3> padding;
  std::array<float, 4> before_row0;
  std::array<float, 4> before_row1;
  std::array<float, 4> after_row0;
  std::array<float, 4> after_row1;
};

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

bool capture_two_pixel_image(
    OBJECT_HANDLE, int frame, bool, void* context,
    void (*callback)(void*, int, const void*, int, int, int)) {
  const std::array<PIXEL_RGBA, 4> pixels{{
      {255, 255, 255, 255}, {255, 255, 255, 255},
      {255, 255, 255, 255}, {255, 255, 255, 255}}};
  callback(context, frame, pixels.data(), 2, 2, 2 * sizeof(PIXEL_RGBA));
  return true;
}

void create_resource(LPCWSTR, const PIXEL_RGBA*, int, int) {}
bool upload_resource(LPCWSTR, const void*, int, int, int, INPUT_PIXEL_FORMAT) {
  return true;
}
bool execute_compute(
    const BYTE*, int, LPCWSTR*, int, LPCWSTR*, int, void*, int,
    int, int, int, ID3D11SamplerState*) {
  return true;
}
bool execute_pixel(
    const BYTE*, int, LPCWSTR, LPCWSTR*, int, void* constants, int,
    ID3D11BlendState*, ID3D11SamplerState*) {
  const auto& value = *static_cast<const MorphConstantsProbe*>(constants);
  before_inverse_row = value.before_row0;
  after_inverse_row = value.after_row0;
  ++pixel_shader_count;
  return true;
}

bool get_split_transform(
    OBJECT_HANDLE endpoint, double, OBJECT_IMAGE_PARAM* param, int size) {
  if (param == nullptr || size != sizeof(*param)) return false;
  *param = {};
  param->sx = endpoint == &split_objects.front() ? before_sx : after_sx;
  param->sy = endpoint == &split_objects.front() ? before_sy : after_sy;
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

void run_endpoint_scale_test() {
  split_render_handle = {};
  split_render_handle.rendering_object_video = &queue_split_capture;
  HOST_APP_TABLE host{};
  host.register_filter_plugin = &ignore_filter_registration;
  host.create_edit_handle = &create_split_edit_handle;
  host.register_event_listener = &ignore_event_registration;
  host.register_clear_cache_handler = &ignore_clear_registration;
  MB_CHECK(morph_bridge::register_morph_bridge(&host));

  EDIT_INFO edit_info{};
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
  OBJECT_INFO object{};
  object.effect_id = 459;
  object.layer = 2;
  object.frame_s = 10;
  object.frame_e = 14;
  before_sx = 1.8F;
  before_sy = 1.2F;
  after_sx = 0.6F;
  after_sy = 1.4F;
  split_objects[2].morph_bridge = false;
  auto* table = morph_bridge::filter_plugin_table();
  auto* progress = static_cast<FILTER_ITEM_TRACK*>(table->items[0]);
  auto* a_x = static_cast<FILTER_ITEM_TRACK*>(table->items[3]);
  auto* a_scale = static_cast<FILTER_ITEM_TRACK*>(table->items[5]);
  auto* a_rotation = static_cast<FILTER_ITEM_TRACK*>(table->items[6]);
  auto* a_aspect = static_cast<FILTER_ITEM_TRACK*>(table->items[7]);
  auto* b_x = static_cast<FILTER_ITEM_TRACK*>(table->items[10]);
  auto* b_scale = static_cast<FILTER_ITEM_TRACK*>(table->items[12]);
  auto* b_rotation = static_cast<FILTER_ITEM_TRACK*>(table->items[13]);
  auto* b_aspect = static_cast<FILTER_ITEM_TRACK*>(table->items[14]);
  a_x->value = 20.0;
  a_scale->value = 150.0;
  a_rotation->value = 30.0;
  a_aspect->value = 20.0;
  b_x->value = 100.0;
  b_scale->value = 80.0;
  b_rotation->value = 90.0;
  b_aspect->value = -25.0;
  void* state = table->func_create(object.effect_id);
  OBJECT_IMAGE_PARAM output_param{};
  FILTER_PROC_VIDEO video{};
  video.scene = &scene;
  video.object = &object;
  video.edit = &edit;
  video.param = &output_param;
  video.userdata = state;
  video.set_image_data = &set_image;
  video.get_output_image_param = &get_split_transform;
  MB_CHECK(table->func_proc_video(&video));
  MB_CHECK_NEAR(output_param.x, 0.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.rz, 0.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.sx, 1.8F, 0.0001F);
  MB_CHECK_NEAR(output_param.sy, 1.2F, 0.0001F);
  progress->value = 50.0;
  MB_CHECK(table->func_proc_video(&video));
  MB_CHECK_NEAR(output_param.x, 0.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.rz, 0.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.sx, 1.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.sy, 1.0F, 0.0001F);
  progress->value = 100.0;
  MB_CHECK(table->func_proc_video(&video));
  MB_CHECK_NEAR(output_param.x, 0.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.rz, 0.0F, 0.0001F);
  MB_CHECK_NEAR(output_param.sx, 0.6F, 0.0001F);
  MB_CHECK_NEAR(output_param.sy, 1.4F, 0.0001F);
  table->func_destroy(object.effect_id, state);
  progress->value = 0.0;
  a_x->value = b_x->value = 0.0;
  a_scale->value = b_scale->value = 100.0;
  a_rotation->value = b_rotation->value = 0.0;
  a_aspect->value = b_aspect->value = 0.0;
  split_objects[2].morph_bridge = true;
  before_sx = before_sy = after_sx = after_sy = 1.0F;
}

void run_sdf_correction_progress_test() {
  split_render_handle = {};
  split_render_handle.rendering_object_video = &capture_two_pixel_image;
  HOST_APP_TABLE host{};
  host.register_filter_plugin = &ignore_filter_registration;
  host.create_edit_handle = &create_split_edit_handle;
  host.register_event_listener = &ignore_event_registration;
  host.register_clear_cache_handler = &ignore_clear_registration;
  MB_CHECK(morph_bridge::register_morph_bridge(&host));

  EDIT_INFO edit_info{};
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
  OBJECT_INFO object{};
  object.effect_id = 460;
  object.layer = 2;
  object.frame_s = 10;
  object.frame_e = 14;
  split_objects[2].morph_bridge = false;
  auto* table = morph_bridge::filter_plugin_table();
  auto* progress = static_cast<FILTER_ITEM_TRACK*>(table->items[0]);
  auto* a_scale = static_cast<FILTER_ITEM_TRACK*>(table->items[5]);
  auto* b_scale = static_cast<FILTER_ITEM_TRACK*>(table->items[12]);
  a_scale->value = 200.0;
  b_scale->value = 300.0;
  void* state = table->func_create(object.effect_id);
  OBJECT_IMAGE_PARAM output_param{};
  FILTER_PROC_VIDEO video{};
  video.scene = &scene;
  video.object = &object;
  video.edit = &edit;
  video.param = &output_param;
  video.userdata = state;
  video.set_image_data = &set_image;
  video.get_output_image_param = &get_split_transform;
  video.create_image_resource = &create_resource;
  video.set_image_resource_data = &upload_resource;
  video.exec_computeshader_data = &execute_compute;
  video.exec_pixelshader_data = &execute_pixel;

  progress->value = 25.0;
  pixel_shader_count = 0;
  MB_CHECK(table->func_proc_video(&video));
  MB_CHECK(pixel_shader_count == 1);
  MB_CHECK_NEAR(before_inverse_row[0], 0.8F, 0.0001F);
  MB_CHECK_NEAR(after_inverse_row[0], 0.4F, 0.0001F);
  progress->value = 75.0;
  MB_CHECK(table->func_proc_video(&video));
  MB_CHECK(pixel_shader_count == 2);
  MB_CHECK_NEAR(before_inverse_row[0], 1.0F / 1.75F, 0.0001F);
  MB_CHECK_NEAR(after_inverse_row[0], 1.0F / 1.5F, 0.0001F);

  table->func_destroy(object.effect_id, state);
  progress->value = 0.0;
  a_scale->value = b_scale->value = 100.0;
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
  run_endpoint_scale_test();
  run_sdf_correction_progress_test();
}

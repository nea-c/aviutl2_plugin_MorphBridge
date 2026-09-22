#include "plugin/filter_object.hpp"
#include "test_support.hpp"

namespace {

int output_width{};
int output_height{};

void set_image(const PIXEL_RGBA*, int width, int height) {
  output_width = width;
  output_height = height;
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
}

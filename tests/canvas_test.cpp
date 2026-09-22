#include "render/canvas.hpp"
#include "test_support.hpp"

using morph_bridge::make_canvas_layout;
using morph_bridge::make_extrapolation_envelope;
using morph_bridge::make_transformed_canvas_layout;

void run_canvas_tests() {
  const auto layout = make_canvas_layout(100, 50, 60, 120, 50, 4);
  MB_CHECK(layout.has_value());
  MB_CHECK(layout->width == 58);
  MB_CHECK(layout->height == 68);
  MB_CHECK(layout->before.width == 50);
  MB_CHECK(layout->before.height == 25);
  MB_CHECK(layout->before.left == 4);
  MB_CHECK(layout->before.top == 21);
  MB_CHECK(layout->after.width == 30);
  MB_CHECK(layout->after.height == 60);
  MB_CHECK(layout->after.left == 14);
  MB_CHECK(layout->after.top == 4);
  MB_CHECK(layout->before_sampling == morph_bridge::SamplingTransform{});
  MB_CHECK(layout->after_sampling == morph_bridge::SamplingTransform{});

  MB_CHECK(!make_canvas_layout(0, 50, 60, 120, 50, 4));
  MB_CHECK(!make_canvas_layout(100, 50, 60, 120, 75, 4));
  MB_CHECK(!make_canvas_layout(100, 50, 60, 120, 50, -1));

  const auto envelope = make_extrapolation_envelope(200, 100, 80, 120, 100);
  MB_CHECK(envelope.has_value());
  MB_CHECK(envelope->distance_limit == 50);
  MB_CHECK(envelope->margin == 54);
  MB_CHECK(make_extrapolation_envelope(20, 10, 10, 20, 100)->distance_limit == 16);
  MB_CHECK(make_extrapolation_envelope(1000, 500, 500, 1000, 100)->distance_limit == 128);
  MB_CHECK(!make_extrapolation_envelope(0, 10, 10, 10, 100));

  morph_bridge::SamplingTransform translated_before;
  translated_before.tx = -30.0F;
  morph_bridge::SamplingTransform translated_after;
  translated_after.tx = 40.0F;
  const auto translated = make_transformed_canvas_layout(
      100, 50, 100, 50, 100, 20, translated_before, translated_after);
  MB_CHECK(translated.has_value());
  MB_CHECK(translated->width == 220);
  MB_CHECK(translated->height == 170);
  MB_CHECK(translated->before.left == 60);
  MB_CHECK(translated->before.top == 60);
  MB_CHECK_NEAR(translated->before_sampling.cx, 110.0F, 0.0001F);
  MB_CHECK_NEAR(translated->before_sampling.cy, 85.0F, 0.0001F);
  MB_CHECK_NEAR(translated->before_sampling.tx, -30.0F, 0.0001F);
  MB_CHECK_NEAR(translated->after_sampling.tx, 40.0F, 0.0001F);

  morph_bridge::SamplingTransform enlarged_before;
  enlarged_before.sx = 2.0F;
  enlarged_before.sy = 0.5F;
  const auto enlarged = make_transformed_canvas_layout(
      100, 50, 100, 50, 100, 20, enlarged_before, {});
  MB_CHECK(enlarged.has_value());
  MB_CHECK(enlarged->width == 240);
  MB_CHECK(enlarged->height == 190);
  MB_CHECK(enlarged->before.left == 70);
  MB_CHECK_NEAR(enlarged->before_sampling.cx, 120.0F, 0.0001F);
}

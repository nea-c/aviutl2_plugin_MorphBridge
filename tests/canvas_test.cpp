#include "render/canvas.hpp"
#include "test_support.hpp"

using morph_bridge::make_canvas_layout;
using morph_bridge::make_extrapolation_envelope;

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
}

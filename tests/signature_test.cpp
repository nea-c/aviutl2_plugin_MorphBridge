#include "cache/signature.hpp"
#include "test_support.hpp"

using morph_bridge::EndpointDescriptor;
using morph_bridge::make_signature;
using morph_bridge::quantize_alpha_threshold;

void run_signature_tests() {
  MB_CHECK(quantize_alpha_threshold(50.01) == 5001);

  EndpointDescriptor a{{1, 2, 0, 9}, 9, "alias-a"};
  EndpointDescriptor b{{3, 2, 20, 29}, 20, "alias-b"};

  const auto base = make_signature(7, 1920, 1080, a, b, 50, 50, 1);
  MB_CHECK(base == make_signature(7, 1920, 1080, a, b, 50, 50, 1));

  auto changed = b;
  changed.alias_utf8 = "alias-b-changed";
  MB_CHECK(base != make_signature(7, 1920, 1080, a, changed, 50, 50, 1));

  changed = b;
  ++changed.frame;
  MB_CHECK(base != make_signature(7, 1920, 1080, a, changed, 50, 50, 1));

  MB_CHECK(base != make_signature(8, 1920, 1080, a, b, 50, 50, 1));
  MB_CHECK(base != make_signature(7, 1280, 1080, a, b, 50, 50, 1));
  MB_CHECK(base != make_signature(7, 1920, 720, a, b, 50, 50, 1));
  MB_CHECK(base != make_signature(7, 1920, 1080, a, b, 51, 50, 1));
  MB_CHECK(base != make_signature(7, 1920, 1080, a, b, 50, 25, 1));
  MB_CHECK(base != make_signature(7, 1920, 1080, a, b, 50, 50, 2));

  auto moved = a;
  ++moved.span.start;
  MB_CHECK(base != make_signature(7, 1920, 1080, moved, b, 50, 50, 1));
}

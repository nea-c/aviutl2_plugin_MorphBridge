#include "domain/neighbor_resolver.hpp"

#include <functional>
#include <optional>

#include "test_support.hpp"

namespace {

using morph_bridge::ObjectSpan;

std::function<std::optional<ObjectSpan>(int, int)> make_finder(
    ObjectSpan before, ObjectSpan after) {
  return [=](int layer, int frame) -> std::optional<ObjectSpan> {
    if (layer != before.layer) return std::nullopt;
    if (frame >= before.start && frame <= before.end) return before;
    if (frame >= after.start && frame <= after.end) return after;
    return std::nullopt;
  };
}

}  // namespace

void run_neighbor_resolver_tests() {
  using morph_bridge::EndpointPair;
  using morph_bridge::resolve_neighbors;

  const ObjectSpan bridge{2, 4, 10, 19};
  const ObjectSpan before{1, 4, 0, 9};

  const auto exact = resolve_neighbors(
      bridge, make_finder(before, ObjectSpan{3, 4, 19, 29}));
  MB_CHECK(exact.has_value());
    const EndpointPair exact_expected{before, {3, 4, 19, 29}, 9, 19};
    MB_CHECK(*exact == exact_expected);

  const auto one_frame_gap = resolve_neighbors(
      bridge, make_finder(before, ObjectSpan{3, 4, 20, 29}));
  MB_CHECK(one_frame_gap.has_value());
  MB_CHECK(one_frame_gap->after_frame == 20);

  MB_CHECK(!resolve_neighbors(
      bridge, make_finder(before, ObjectSpan{3, 4, 21, 29})).has_value());

  MB_CHECK(!resolve_neighbors(
      bridge, make_finder(ObjectSpan{1, 3, 0, 9}, ObjectSpan{3, 4, 19, 29}))
                .has_value());

  MB_CHECK(!resolve_neighbors(
      bridge, make_finder(bridge, ObjectSpan{3, 4, 19, 29})).has_value());

  MB_CHECK(!resolve_neighbors(
      ObjectSpan{2, 4, 0, 9},
      make_finder(ObjectSpan{1, 4, 0, 0}, ObjectSpan{3, 4, 9, 19}))
                .has_value());
}

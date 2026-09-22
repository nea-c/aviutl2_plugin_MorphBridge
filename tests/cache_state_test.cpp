#include "cache/cache_state.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <utility>

using morph_bridge::CacheAction;
using morph_bridge::EndpointSignature;
using morph_bridge::PreparationState;
using morph_bridge::PreparedEndpoints;
using morph_bridge::RgbaImage;

namespace {

PreparedEndpoints images_with_marker(const std::byte marker) {
  RgbaImage before{1, 1, {marker, marker, marker, marker}};
  RgbaImage after{1, 1, {marker, marker, marker, marker}};
  return {std::move(before), std::move(after)};
}

}  // namespace

void run_cache_state_tests() {
  const EndpointSignature sig_a{1, 2};
  const EndpointSignature sig_b{3, 4};

  PreparationState state;
  MB_CHECK(state.observe(1, sig_a).action == CacheAction::StartCapture);
  const auto request_a = state.active_request();
  MB_CHECK(request_a != 0);
  MB_CHECK(state.observe(2, sig_a).action == CacheAction::WaitForCapture);
  MB_CHECK(!state.complete(request_a, sig_a, images_with_marker(std::byte{0x11})));
  MB_CHECK(state.observe(2, sig_a).action == CacheAction::StartCapture);
  const auto request_a_current = state.active_request();
  MB_CHECK(request_a_current > request_a);
  MB_CHECK(state.complete(
      request_a_current, sig_a, images_with_marker(std::byte{0x12})));
  MB_CHECK(state.observe(2, sig_a).action == CacheAction::Keep);
  MB_CHECK(state.ready() != nullptr);

  MB_CHECK(state.observe(3, sig_a).action == CacheAction::StartCapture);
  const auto request_a_next_generation = state.active_request();
  MB_CHECK(state.complete(
      request_a_next_generation, sig_a, images_with_marker(std::byte{0x13})));

  MB_CHECK(state.observe(4, sig_b).action == CacheAction::StartCapture);
  const auto request_b = state.active_request();
  MB_CHECK(request_b > request_a);
  MB_CHECK(!state.complete(request_a, sig_a, images_with_marker(std::byte{0x22})));
  MB_CHECK(state.complete(request_b, sig_b, images_with_marker(std::byte{0x33})));
  MB_CHECK(state.observe(4, sig_b).action == CacheAction::Keep);

  PreparationState coalesced;
  MB_CHECK(coalesced.observe(1, sig_a).action == CacheAction::StartCapture);
  const auto stale_request = coalesced.active_request();
  MB_CHECK(coalesced.observe(2, sig_b).action == CacheAction::WaitForCapture);
  MB_CHECK(!coalesced.complete(stale_request, sig_a, images_with_marker(std::byte{0x44})));
  MB_CHECK(coalesced.observe(2, sig_b).action == CacheAction::StartCapture);
}

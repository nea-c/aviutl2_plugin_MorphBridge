#include "capture/endpoint_capture.hpp"
#include "test_support.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using morph_bridge::CaptureError;
using morph_bridge::CaptureResult;
using morph_bridge::EndpointCapture;
using morph_bridge::EndpointSignature;
using morph_bridge::RgbaImage;
using morph_bridge::copy_rgba_rows;

namespace {

struct PendingRender {
  void* object{};
  int frame{};
  bool apply_effect{};
  void* param{};
  morph_bridge::RenderingVideoCallback callback{};
};

void run_copy_tests() {
  const std::array<std::byte, 24> source{
      std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3},
      std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7},
      std::byte{99}, std::byte{99}, std::byte{99}, std::byte{99},
      std::byte{8}, std::byte{9}, std::byte{10}, std::byte{11},
      std::byte{12}, std::byte{13}, std::byte{14}, std::byte{15},
      std::byte{99}, std::byte{99}, std::byte{99}, std::byte{99}};

  const auto copied = copy_rgba_rows(source.data(), 2, 2, 12);
  const auto* image = std::get_if<RgbaImage>(&copied);
  MB_CHECK(image != nullptr);
  MB_CHECK(image->width == 2);
  MB_CHECK(image->height == 2);
  MB_CHECK(image->pixels.size() == 16);
  MB_CHECK(image->pixels[8] == source[12]);

  MB_CHECK(std::holds_alternative<CaptureError>(copy_rgba_rows(nullptr, 2, 2, 8)));
  MB_CHECK(std::holds_alternative<CaptureError>(
      copy_rgba_rows(source.data(), 0, 2, 8)));
  MB_CHECK(std::holds_alternative<CaptureError>(
      copy_rgba_rows(source.data(), 2, 2, 7)));
  MB_CHECK(std::holds_alternative<CaptureError>(copy_rgba_rows(
      source.data(), std::numeric_limits<int>::max(),
      std::numeric_limits<int>::max(), std::numeric_limits<int>::max())));
}

void run_coordination_tests() {
  std::vector<PendingRender> pending;
  EndpointCapture capture{[&](void* object, int frame, bool apply_effect, void* param,
                              morph_bridge::RenderingVideoCallback callback) {
    pending.push_back({object, frame, apply_effect, param, callback});
    return true;
  }};

  int before_object = 1;
  int after_object = 2;
  std::optional<CaptureResult> result;
  capture.request(&before_object, 9, &after_object, 20, 42, {11, 12},
                  [&](CaptureResult value) { result = std::move(value); });

  MB_CHECK(pending.size() == 2);
  MB_CHECK(pending[0].object == &before_object);
  MB_CHECK(pending[0].frame == 9);
  MB_CHECK(pending[0].apply_effect);
  MB_CHECK(pending[1].object == &after_object);
  MB_CHECK(pending[1].frame == 20);
  MB_CHECK(pending[1].apply_effect);

  const std::array<std::byte, 4> pixel{
      std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
  pending[1].callback(pending[1].param, 20, pixel.data(), 1, 1, 4);
  MB_CHECK(!result.has_value());
  pending[0].callback(pending[0].param, 9, pixel.data(), 1, 1, 4);
  MB_CHECK(result.has_value());
  MB_CHECK(result->request_id == 42);
  const EndpointSignature expected_signature{11, 12};
  MB_CHECK(result->signature == expected_signature);
  MB_CHECK(result->images.has_value());

  int completion_count = 0;
  EndpointCapture rejected{[](void*, int, bool, void*, morph_bridge::RenderingVideoCallback) {
    return false;
  }};
  rejected.request(&before_object, 9, &after_object, 20, 43, {13, 14},
                   [&](CaptureResult value) {
    ++completion_count;
    MB_CHECK(value.error == CaptureError::RequestRejected);
  });
  MB_CHECK(completion_count == 1);
}

}  // namespace

void run_endpoint_capture_tests() {
  run_copy_tests();
  run_coordination_tests();
}

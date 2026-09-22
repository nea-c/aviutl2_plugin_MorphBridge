#include "render/gpu_renderer.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using morph_bridge::CanvasLayout;
using morph_bridge::EndpointSignature;
using morph_bridge::GpuRenderer;
using morph_bridge::GpuRenderRequest;
using morph_bridge::PreparedEndpoints;
using morph_bridge::RenderError;
using morph_bridge::RgbaImage;
using morph_bridge::build_gpu_resource_plan;

namespace {

struct Recorder {
  std::vector<std::wstring> calls;
  int width{};
  int height{};
  bool warm{};
  int fail_upload_at{-1};
  int upload_count{};
};

Recorder* active_recorder{};

void set_image(const PIXEL_RGBA*, int width, int height) {
  active_recorder->calls.push_back(L"object");
  active_recorder->width = width;
  active_recorder->height = height;
}

bool resource_size(LPCWSTR resource, int* width, int* height) {
  active_recorder->calls.push_back(std::wstring{L"size:"} + resource);
  if (!active_recorder->warm) return false;
  *width = active_recorder->width;
  *height = active_recorder->height;
  return true;
}

bool upload(
    LPCWSTR resource, const void*, int, int, int, INPUT_PIXEL_FORMAT) {
  active_recorder->calls.push_back(std::wstring{L"upload:"} + resource);
  const int index = active_recorder->upload_count++;
  return index != active_recorder->fail_upload_at;
}

bool compute(
    const BYTE*, int, LPCWSTR* targets, int, LPCWSTR*, int, void*, int,
    int, int, int, ID3D11SamplerState*) {
  active_recorder->calls.push_back(std::wstring{L"compute:"} + targets[0]);
  return true;
}

bool pixel(
    const BYTE*, int, LPCWSTR target, LPCWSTR*, int, void*, int,
    ID3D11BlendState*, ID3D11SamplerState*) {
  active_recorder->calls.push_back(std::wstring{L"pixel:"} + target);
  return true;
}

FILTER_PROC_VIDEO make_video() {
  FILTER_PROC_VIDEO video{};
  video.set_image_data = &set_image;
  video.get_image_resource_size = &resource_size;
  video.set_image_resource_data = &upload;
  video.exec_computeshader_data = &compute;
  video.exec_pixelshader_data = &pixel;
  return video;
}

PreparedEndpoints endpoints() {
  RgbaImage before{2, 2, std::vector<std::byte>(16, std::byte{0xff})};
  RgbaImage after{2, 2, std::vector<std::byte>(16, std::byte{0x80})};
  return {std::move(before), std::move(after)};
}

CanvasLayout layout() {
  return {4, 4, {1, 1, 2, 2}, {1, 1, 2, 2}, {}, {}};
}

GpuRenderRequest request(FILTER_PROC_VIDEO& video, const PreparedEndpoints& images) {
  GpuRenderRequest value;
  value.video = &video;
  value.endpoints = &images;
  value.layout = layout();
  value.effect_id = 0x2a;
  value.signature = {1, 2};
  value.progress = 0.5F;
  value.alpha_threshold = 0.5F;
  value.color = {1.0F, 0.5F, 0.25F, 1.0F};
  return value;
}

}  // namespace

void run_gpu_renderer_contract_tests() {
  const auto first = build_gpu_resource_plan(0x2a, {1, 2});
  const auto same = build_gpu_resource_plan(0x2a, {1, 2});
  const auto changed = build_gpu_resource_plan(0x2a, {1, 3});
  MB_CHECK(first == same);
  MB_CHECK(first.before.upload != first.after.upload);
  MB_CHECK(first.before.final_sdf != first.after.final_sdf);
  MB_CHECK(first.before.final_sdf != changed.before.final_sdf);
  for (const auto& jump : first.before.jumps) {
    MB_CHECK(jump.source != jump.target);
  }

  auto images = endpoints();
  auto video = make_video();
  Recorder cold;
  active_recorder = &cold;
  const auto cold_result = GpuRenderer{}.render(request(video, images));
  MB_CHECK(cold_result.error == RenderError::None);
  MB_CHECK(cold_result.rebuilt);
  MB_CHECK(cold.calls.front() == L"object");
  MB_CHECK(cold.calls[cold.calls.size() - 1] == L"pixel:object");
  MB_CHECK(cold.upload_count == 2);

  Recorder warm;
  warm.warm = true;
  active_recorder = &warm;
  const auto warm_result = GpuRenderer{}.render(request(video, images));
  MB_CHECK(warm_result.error == RenderError::None);
  MB_CHECK(!warm_result.rebuilt);
  MB_CHECK(warm.upload_count == 0);
  MB_CHECK(warm.calls.front() == L"object");
  MB_CHECK(warm.calls.back() == L"pixel:object");

  Recorder failed;
  failed.fail_upload_at = 0;
  active_recorder = &failed;
  const auto failed_result = GpuRenderer{}.render(request(video, images));
  MB_CHECK(failed_result.error == RenderError::UploadFailed);
}

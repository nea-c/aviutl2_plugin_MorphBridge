#include "render/gpu_renderer.hpp"
#include "test_support.hpp"

#include <cstddef>
#include <cstdint>
#include <array>
#include <set>
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
  int size_query_count{};
  int fail_upload_at{-1};
  int upload_count{};
  std::vector<std::wstring> pixel_sources;
};

Recorder* active_recorder{};
std::set<std::wstring> created_resources;

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
MorphConstantsProbe captured_constants{};

void set_image(const PIXEL_RGBA*, int width, int height) {
  active_recorder->calls.push_back(L"object");
  active_recorder->width = width;
  active_recorder->height = height;
}

bool resource_size(LPCWSTR resource, int* width, int* height) {
  active_recorder->calls.push_back(std::wstring{L"size:"} + resource);
  ++active_recorder->size_query_count;
  if (!active_recorder->warm) return false;
  *width = active_recorder->width;
  *height = active_recorder->height;
  return true;
}

bool upload(
    LPCWSTR resource, const void*, int, int, int, INPUT_PIXEL_FORMAT) {
  active_recorder->calls.push_back(std::wstring{L"upload:"} + resource);
  const int index = active_recorder->upload_count++;
  return std::wstring{resource}.starts_with(L"resource:") &&
         index != active_recorder->fail_upload_at;
}

void create_resource(LPCWSTR resource, const PIXEL_RGBA*, int, int) {
  active_recorder->calls.push_back(std::wstring{L"create:"} + resource);
  created_resources.insert(resource);
}

bool compute(
    const BYTE*, int, LPCWSTR* targets, int, LPCWSTR*, int, void*, int,
    int, int, int, ID3D11SamplerState*) {
  active_recorder->calls.push_back(std::wstring{L"compute:"} + targets[0]);
  return created_resources.contains(targets[0]);
}

bool pixel(
    const BYTE*, int, LPCWSTR target, LPCWSTR* sources, int source_count,
    void* constants, int,
    ID3D11BlendState*, ID3D11SamplerState*) {
  active_recorder->calls.push_back(std::wstring{L"pixel:"} + target);
  active_recorder->pixel_sources.assign(sources, sources + source_count);
  captured_constants = *static_cast<const MorphConstantsProbe*>(constants);
  return true;
}

FILTER_PROC_VIDEO make_video() {
  FILTER_PROC_VIDEO video{};
  video.set_image_data = &set_image;
  video.get_image_resource_size = &resource_size;
  video.create_image_resource = &create_resource;
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
  value.cache_generation = 7;
  value.progress = 1.25F;
  value.alpha_threshold = 0.5F;
  value.extrapolation_limit = 32.0F;
  value.color = {1.0F, 0.5F, 0.25F, 1.0F};
  value.before_sampling = {5.0F, -10.0F, 0.0F, 2.0F, 0.5F, 100.0F, 50.0F};
  return value;
}

}  // namespace

void run_gpu_renderer_contract_tests() {
  created_resources.clear();
  const auto first = build_gpu_resource_plan(0x2a, {1, 2});
  const auto same = build_gpu_resource_plan(0x2a, {1, 2});
  const auto changed = build_gpu_resource_plan(0x2a, {1, 3});
  MB_CHECK(first == same);
  MB_CHECK(first.before.upload.starts_with(L"resource:"));
  MB_CHECK(first.after.upload.starts_with(L"resource:"));
  MB_CHECK(first.before.upload != first.after.upload);
  MB_CHECK(first.before.final_sdf != first.after.final_sdf);
  MB_CHECK(first.before.final_sdf != changed.before.final_sdf);
  for (const auto& jump : first.before.jumps) {
    MB_CHECK(jump.source != jump.target);
  }

  auto images = endpoints();
  auto video = make_video();
  GpuRenderer renderer;
  Recorder cold;
  active_recorder = &cold;
  const auto cold_result = renderer.render(request(video, images));
  MB_CHECK(cold_result.error == RenderError::None);
  MB_CHECK(cold_result.rebuilt);
  MB_CHECK(cold.size_query_count == 0);
  MB_CHECK(cold.calls.front() == L"object");
  MB_CHECK(cold.calls[cold.calls.size() - 1] == L"pixel:object");
  MB_CHECK(cold.upload_count == 2);
  MB_CHECK(created_resources.size() == 8);
  MB_CHECK(cold.pixel_sources.size() == 4);
  MB_CHECK(cold.pixel_sources[0] == first.before.final_sdf);
  MB_CHECK(cold.pixel_sources[1] == first.after.final_sdf);
  MB_CHECK(cold.pixel_sources[2] == first.before.upload);
  MB_CHECK(cold.pixel_sources[3] == first.after.upload);
  MB_CHECK_NEAR(captured_constants.progress, 1.25F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.extrapolation_limit, 32.0F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.before_row0[0], 0.5F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.before_row0[1], 0.0F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.before_row0[2], 47.5F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.before_row1[0], 0.0F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.before_row1[1], 2.0F, 0.0001F);
  MB_CHECK_NEAR(captured_constants.before_row1[2], -30.0F, 0.0001F);

  Recorder warm;
  warm.warm = true;
  active_recorder = &warm;
  const auto warm_result = renderer.render(request(video, images));
  MB_CHECK(warm_result.error == RenderError::None);
  MB_CHECK(warm_result.rebuilt);
  MB_CHECK(warm.upload_count == 2);
  MB_CHECK(warm.size_query_count == 0);
  MB_CHECK(warm.calls.front() == L"object");
  MB_CHECK(warm.calls.back() == L"pixel:object");

  Recorder cleared;
  active_recorder = &cleared;
  auto after_clear = request(video, images);
  after_clear.cache_generation = 8;
  const auto cleared_result = renderer.render(after_clear);
  MB_CHECK(cleared_result.error == RenderError::None);
  MB_CHECK(cleared_result.rebuilt);
  MB_CHECK(cleared.size_query_count == 0);
  MB_CHECK(cleared.upload_count == 2);

  Recorder failed;
  failed.fail_upload_at = 0;
  active_recorder = &failed;
  const auto failed_result = GpuRenderer{}.render(request(video, images));
  MB_CHECK(failed_result.error == RenderError::UploadFailed);
}

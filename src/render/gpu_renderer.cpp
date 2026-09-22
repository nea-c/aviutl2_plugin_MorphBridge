#include "render/gpu_renderer.hpp"

#include "shaders.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cwchar>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace morph_bridge {
namespace {

std::wstring fixed_hex(const std::uint64_t value) {
  std::array<wchar_t, 17> buffer{};
  std::swprintf(buffer.data(), buffer.size(), L"%016llx",
                static_cast<unsigned long long>(value));
  return buffer.data();
}

std::vector<std::byte> place_image(
    const RgbaImage& image, const CanvasLayout& layout, const PlacementRect& rect) {
  const auto canvas_bytes = static_cast<std::size_t>(layout.width) *
                            static_cast<std::size_t>(layout.height) * 4U;
  std::vector<std::byte> output(canvas_bytes, std::byte{0});
  if (image.width <= 0 || image.height <= 0 ||
      image.pixels.size() != static_cast<std::size_t>(image.width) *
                                 static_cast<std::size_t>(image.height) * 4U) {
    return {};
  }
  for (int y = 0; y < rect.height; ++y) {
    const int source_y = std::min(image.height - 1, y * image.height / rect.height);
    for (int x = 0; x < rect.width; ++x) {
      const int source_x = std::min(image.width - 1, x * image.width / rect.width);
      const auto source_offset =
          (static_cast<std::size_t>(source_y) * image.width + source_x) * 4U;
      const auto target_offset =
          (static_cast<std::size_t>(rect.top + y) * layout.width + rect.left + x) * 4U;
      std::copy_n(image.pixels.data() + source_offset, 4,
                  output.data() + target_offset);
    }
  }
  return output;
}

struct alignas(16) SeedConstants {
  std::uint32_t width;
  std::uint32_t height;
  float alpha_threshold;
  std::uint32_t padding{};
};

struct alignas(16) JumpConstants {
  std::uint32_t width;
  std::uint32_t height;
  std::uint32_t step;
  std::uint32_t padding{};
};

struct alignas(16) FinalizeConstants {
  std::uint32_t width;
  std::uint32_t height;
  float alpha_threshold;
  float maximum_distance;
};

struct alignas(16) MorphConstants {
  std::array<float, 4> color;
  float progress;
  float maximum_distance;
  float feather_width;
  float opacity;
  std::array<float, 4> before_row0;
  std::array<float, 4> before_row1;
  std::array<float, 4> after_row0;
  std::array<float, 4> after_row1;
};

static_assert(sizeof(SeedConstants) % 16 == 0);
static_assert(sizeof(JumpConstants) % 16 == 0);
static_assert(sizeof(FinalizeConstants) % 16 == 0);
static_assert(sizeof(MorphConstants) % 16 == 0);

bool resource_ready(
    FILTER_PROC_VIDEO& video, const std::wstring& name, const CanvasLayout& layout) {
  int width = 0;
  int height = 0;
  return video.get_image_resource_size(name.c_str(), &width, &height) &&
         width == layout.width && height == layout.height;
}

bool execute_compute(
    FILTER_PROC_VIDEO& video,
    const unsigned char* shader,
    const std::size_t shader_size,
    const std::wstring& target,
    const std::vector<std::wstring>& resources,
    void* constants,
    const int constants_size,
    const SdfDispatchPlan& plan) {
  LPCWSTR target_pointer = target.c_str();
  std::vector<LPCWSTR> resource_pointers;
  resource_pointers.reserve(resources.size());
  for (const auto& resource : resources) {
    resource_pointers.push_back(resource.c_str());
  }
  return video.exec_computeshader_data(
      shader, static_cast<int>(shader_size), &target_pointer, 1,
      resource_pointers.data(), static_cast<int>(resource_pointers.size()),
      constants, constants_size, plan.groups_x, plan.groups_y, 1, nullptr);
}

bool build_endpoint_sdf(
    FILTER_PROC_VIDEO& video,
    const SdfDispatchPlan& plan,
    const float threshold,
    const float maximum_distance) {
  SeedConstants seed{
      static_cast<std::uint32_t>(plan.width),
      static_cast<std::uint32_t>(plan.height), threshold};
  if (!execute_compute(video, shaders::seed_cso, shaders::seed_cso_size,
                       plan.seed, {plan.upload}, &seed, sizeof(seed), plan)) {
    return false;
  }
  for (const auto& jump : plan.jumps) {
    JumpConstants constants{
        static_cast<std::uint32_t>(plan.width),
        static_cast<std::uint32_t>(plan.height),
        static_cast<std::uint32_t>(jump.step)};
    if (!execute_compute(video, shaders::jump_flood_cso,
                         shaders::jump_flood_cso_size, jump.target,
                         {jump.source}, &constants, sizeof(constants), plan)) {
      return false;
    }
  }
  FinalizeConstants finalize{
      static_cast<std::uint32_t>(plan.width),
      static_cast<std::uint32_t>(plan.height), threshold, maximum_distance};
  return execute_compute(video, shaders::finalize_cso, shaders::finalize_cso_size,
                         plan.final_sdf, {plan.final_seed, plan.upload},
                         &finalize, sizeof(finalize), plan);
}

std::pair<std::array<float, 4>, std::array<float, 4>> inverse_rows(
    const SamplingTransform& transform) {
  constexpr float pi = 3.14159265358979323846F;
  const float radians = transform.rotation * pi / 180.0F;
  const float cosine = std::cos(radians);
  const float sine = std::sin(radians);
  const float sx = std::max(std::abs(transform.sx), 1.0e-6F);
  const float sy = std::max(std::abs(transform.sy), 1.0e-6F);
  const std::array<float, 4> row0{
      cosine / sx, sine / sx,
      -(cosine * transform.tx + sine * transform.ty) / sx, 0.0F};
  const std::array<float, 4> row1{
      -sine / sy, cosine / sy,
      -(-sine * transform.tx + cosine * transform.ty) / sy, 0.0F};
  return {row0, row1};
}

}  // namespace

GpuResourcePlan build_gpu_resource_plan(
    const std::uint64_t effect_id, const EndpointSignature signature) {
  const std::wstring base = L"cache:MorphBridge." + fixed_hex(effect_id) + L"." +
                            fixed_hex(signature.high) + fixed_hex(signature.low);
  return {
      *build_sdf_plan(1, 1, base + L".a"),
      *build_sdf_plan(1, 1, base + L".b")};
}

RenderResult GpuRenderer::render(const GpuRenderRequest& request) const {
  if (request.video == nullptr || request.endpoints == nullptr ||
      request.layout.width <= 0 || request.layout.height <= 0 ||
      request.video->set_image_data == nullptr ||
      request.video->get_image_resource_size == nullptr ||
      request.video->set_image_resource_data == nullptr ||
      request.video->exec_computeshader_data == nullptr ||
      request.video->exec_pixelshader_data == nullptr) {
    return {RenderError::InvalidRequest, false};
  }

  auto& video = *request.video;
  video.set_image_data(nullptr, request.layout.width, request.layout.height);
  const auto resources = build_gpu_resource_plan(request.effect_id, request.signature);
  auto before_plan = build_sdf_plan(
      request.layout.width, request.layout.height, resources.before.prefix);
  auto after_plan = build_sdf_plan(
      request.layout.width, request.layout.height, resources.after.prefix);
  if (!before_plan || !after_plan) {
    return {RenderError::InvalidRequest, false};
  }

  const bool warm =
      resource_ready(video, before_plan->final_sdf, request.layout) &&
      resource_ready(video, after_plan->final_sdf, request.layout);
  const float maximum_distance = std::hypot(
      static_cast<float>(request.layout.width),
      static_cast<float>(request.layout.height));

  if (!warm) {
    const auto before_pixels = place_image(
        request.endpoints->before, request.layout, request.layout.before);
    const auto after_pixels = place_image(
        request.endpoints->after, request.layout, request.layout.after);
    if (before_pixels.empty() || after_pixels.empty()) {
      return {RenderError::InvalidRequest, false};
    }
    const int pitch = request.layout.width * 4;
    if (!video.set_image_resource_data(
            before_plan->upload.c_str(), before_pixels.data(),
            request.layout.width, request.layout.height, pitch,
            INPUT_PIXEL_FORMAT::RGBA) ||
        !video.set_image_resource_data(
            after_plan->upload.c_str(), after_pixels.data(),
            request.layout.width, request.layout.height, pitch,
            INPUT_PIXEL_FORMAT::RGBA)) {
      return {RenderError::UploadFailed, false};
    }
    const float threshold = std::clamp(request.alpha_threshold, 0.0F, 1.0F);
    if (!build_endpoint_sdf(video, *before_plan, threshold, maximum_distance) ||
        !build_endpoint_sdf(video, *after_plan, threshold, maximum_distance)) {
      return {RenderError::ComputeFailed, false};
    }
  }

  const auto [before_row0, before_row1] = inverse_rows(request.before_sampling);
  const auto [after_row0, after_row1] = inverse_rows(request.after_sampling);
  MorphConstants constants{
      request.color,
      std::clamp(request.progress, 0.0F, 1.0F),
      maximum_distance,
      1.0F,
      std::clamp(request.color[3], 0.0F, 1.0F),
      before_row0,
      before_row1,
      after_row0,
      after_row1};
  std::array<LPCWSTR, 2> sdf_resources{
      before_plan->final_sdf.c_str(), after_plan->final_sdf.c_str()};
  if (!video.exec_pixelshader_data(
          shaders::morph_cso, static_cast<int>(shaders::morph_cso_size),
          L"object", sdf_resources.data(), static_cast<int>(sdf_resources.size()),
          &constants, sizeof(constants), nullptr, nullptr)) {
    return {RenderError::PixelFailed, !warm};
  }
  return {RenderError::None, !warm};
}

}  // namespace morph_bridge

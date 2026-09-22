#pragma once

#include <cstdint>
#include <windows.h>

#include "plugin2.h"
#include "filter2.h"

#include "cache/cache_state.hpp"
#include "cache/signature.hpp"
#include "domain/types.hpp"
#include "render/canvas.hpp"
#include "render/sdf_plan.hpp"

#include <array>
#include <mutex>
#include <optional>

namespace morph_bridge {

struct GpuResourcePlan {
  SdfDispatchPlan before;
  SdfDispatchPlan after;
  friend bool operator==(const GpuResourcePlan&, const GpuResourcePlan&) = default;
};

[[nodiscard]] GpuResourcePlan build_gpu_resource_plan(
    std::uint64_t effect_id, EndpointSignature signature);

enum class RenderError {
  None,
  InvalidRequest,
  UploadFailed,
  ComputeFailed,
  PixelFailed,
};

struct RenderResult {
  RenderError error{RenderError::None};
  bool rebuilt{};
};

struct GpuRenderRequest {
  FILTER_PROC_VIDEO* video{};
  const PreparedEndpoints* endpoints{};
  CanvasLayout layout;
  std::uint64_t effect_id{};
  EndpointSignature signature;
  std::uint64_t cache_generation{};
  float progress{};
  float alpha_threshold{0.5F};
  float extrapolation_limit{};
  std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
  SamplingTransform before_sampling;
  SamplingTransform after_sampling;
};

class GpuRenderer {
 public:
  [[nodiscard]] RenderResult render(const GpuRenderRequest& request);

 private:
  struct CacheIdentity {
    std::uint64_t effect_id{};
    EndpointSignature signature;
    std::uint64_t cache_generation{};
    int width{};
    int height{};
    friend bool operator==(const CacheIdentity&, const CacheIdentity&) = default;
  };

  std::mutex mutex_;
  std::optional<CacheIdentity> expected_cache_;
};

}  // namespace morph_bridge

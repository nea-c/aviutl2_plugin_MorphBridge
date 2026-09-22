#pragma once

#include "cache/cache_state.hpp"
#include "cache/signature.hpp"
#include "domain/types.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <variant>

namespace morph_bridge {

enum class CaptureError { None, InvalidGeometry, AllocationFailed, RequestRejected };

using ImageCopyResult = std::variant<RgbaImage, CaptureError>;

[[nodiscard]] ImageCopyResult copy_rgba_rows(
    const void* source, int width, int height, int pitch);

using RenderingVideoCallback =
    void (*)(void* param, int frame, const void* buffer, int width, int height, int pitch);

using RenderingVideoRequest = std::function<bool(
    void* object,
    int frame,
    bool apply_effect,
    void* param,
    RenderingVideoCallback callback)>;

struct CaptureResult {
  std::uint64_t request_id{};
  EndpointSignature signature;
  std::optional<PreparedEndpoints> images;
  CaptureError error{CaptureError::None};
};

class EndpointCapture {
 public:
  using Completion = std::function<void(CaptureResult)>;

  explicit EndpointCapture(RenderingVideoRequest request);

  void request(
      void* before_object,
      int before_frame,
      void* after_object,
      int after_frame,
      std::uint64_t request_id,
      EndpointSignature signature,
      Completion completion) const;

 private:
  RenderingVideoRequest request_;
};

}  // namespace morph_bridge

#include "capture/endpoint_capture.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

namespace morph_bridge {

ImageCopyResult copy_rgba_rows(
    const void* source, const int width, const int height, const int pitch) {
  if (source == nullptr || width <= 0 || height <= 0 || pitch <= 0) {
    return CaptureError::InvalidGeometry;
  }

  constexpr std::size_t bytes_per_pixel = 4;
  const auto unsigned_width = static_cast<std::size_t>(width);
  const auto unsigned_height = static_cast<std::size_t>(height);
  if (unsigned_width > std::numeric_limits<std::size_t>::max() / bytes_per_pixel) {
    return CaptureError::InvalidGeometry;
  }
  const auto row_bytes = unsigned_width * bytes_per_pixel;
  if (row_bytes > static_cast<std::size_t>(pitch) ||
      unsigned_height > std::numeric_limits<std::size_t>::max() / row_bytes) {
    return CaptureError::InvalidGeometry;
  }
  const auto image_bytes = row_bytes * unsigned_height;

  try {
    RgbaImage image{width, height, std::vector<std::byte>(image_bytes)};
    const auto* input = static_cast<const std::byte*>(source);
    for (std::size_t row = 0; row < unsigned_height; ++row) {
      std::memcpy(image.pixels.data() + row * row_bytes,
                  input + row * static_cast<std::size_t>(pitch), row_bytes);
    }
    return image;
  } catch (const std::bad_alloc&) {
    return CaptureError::AllocationFailed;
  }
}

namespace {

struct CaptureJob {
  std::uint64_t request_id{};
  EndpointSignature signature;
  EndpointCapture::Completion completion;
  std::mutex mutex;
  std::array<std::optional<RgbaImage>, 2> images;
  int remaining{2};
  bool completed{};

  void finish_error(const CaptureError error) {
    EndpointCapture::Completion notify;
    {
      std::scoped_lock lock{mutex};
      if (completed) {
        return;
      }
      completed = true;
      notify = completion;
    }
    notify(CaptureResult{request_id, signature, std::nullopt, error});
  }

  void accept(const std::size_t index, ImageCopyResult copied) {
    if (const auto* error = std::get_if<CaptureError>(&copied)) {
      finish_error(*error);
      return;
    }

    EndpointCapture::Completion notify;
    std::optional<PreparedEndpoints> prepared;
    {
      std::scoped_lock lock{mutex};
      if (completed) {
        return;
      }
      images[index] = std::move(std::get<RgbaImage>(copied));
      --remaining;
      if (remaining != 0) {
        return;
      }
      completed = true;
      prepared.emplace(
          PreparedEndpoints{std::move(*images[0]), std::move(*images[1])});
      notify = completion;
    }
    notify(CaptureResult{request_id, signature, std::move(prepared), CaptureError::None});
  }
};

struct CallbackContext {
  std::shared_ptr<CaptureJob> job;
  std::size_t index{};
};

void receive_image(
    void* param,
    int,
    const void* buffer,
    const int width,
    const int height,
    const int pitch) {
  std::unique_ptr<CallbackContext> context{static_cast<CallbackContext*>(param)};
  context->job->accept(context->index, copy_rgba_rows(buffer, width, height, pitch));
}

bool queue_one(
    const RenderingVideoRequest& request,
    void* object,
    const int frame,
    const std::shared_ptr<CaptureJob>& job,
    const std::size_t index) {
  auto context = std::make_unique<CallbackContext>(CallbackContext{job, index});
  auto* raw_context = context.release();
  bool queued = false;
  try {
    queued = request(object, frame, true, raw_context, &receive_image);
  } catch (...) {
    delete raw_context;
    throw;
  }
  if (!queued) {
    delete raw_context;
  }
  return queued;
}

}  // namespace

EndpointCapture::EndpointCapture(RenderingVideoRequest request)
    : request_(std::move(request)) {}

void EndpointCapture::request(
    void* before_object,
    const int before_frame,
    void* after_object,
    const int after_frame,
    const std::uint64_t request_id,
    const EndpointSignature signature,
    Completion completion) const {
  auto job = std::make_shared<CaptureJob>();
  job->request_id = request_id;
  job->signature = signature;
  job->completion = std::move(completion);

  if (!request_ ||
      !queue_one(request_, before_object, before_frame, job, 0)) {
    job->finish_error(CaptureError::RequestRejected);
    return;
  }
  if (!queue_one(request_, after_object, after_frame, job, 1)) {
    job->finish_error(CaptureError::RequestRejected);
  }
}

}  // namespace morph_bridge

#include "plugin/filter_object.hpp"

#include "cache/cache_state.hpp"
#include "cache/signature.hpp"
#include "capture/endpoint_capture.hpp"
#include "domain/neighbor_resolver.hpp"
#include "render/canvas.hpp"
#include "render/gpu_renderer.hpp"
#include "render/standard_transform.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <string>
#include <utility>

namespace morph_bridge {
namespace {

std::atomic_uint64_t edit_generation{1};
std::atomic_uint64_t clear_generation{1};
EDIT_HANDLE* edit_handle{};

FILTER_ITEM_TRACK progress{L"Progress", 0.0, 0.0, 100.0, 0.01};
FILTER_ITEM_COLOR color{L"Color", 0xffffff};
FILTER_ITEM_TRACK alpha_threshold{L"Alpha threshold", 50.0, 0.0, 100.0, 0.1};
FILTER_ITEM_SELECT::ITEM scale_items[]{
    {L"25%", 25}, {L"50%", 50}, {L"100%", 100}, {nullptr, 0}};
FILTER_ITEM_SELECT sdf_scale{L"SDF scale", 50, scale_items};

FILTER_ITEM_GROUP a_corrections{L"A Corrections"};
FILTER_ITEM_TRACK a_x{L"Position X", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK a_y{L"Position Y", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK a_z{L"Position Z", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK a_cx{L"Center X", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK a_cy{L"Center Y", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK a_cz{L"Center Z", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK a_rx{L"Rotation X", 0.0, -3600.0, 3600.0, 0.1};
FILTER_ITEM_TRACK a_ry{L"Rotation Y", 0.0, -3600.0, 3600.0, 0.1};
FILTER_ITEM_TRACK a_rz{L"Rotation Z", 0.0, -3600.0, 3600.0, 0.1};
FILTER_ITEM_TRACK a_scale{L"Scale", 0.0, -99.0, 1000.0, 0.1};
FILTER_ITEM_TRACK a_aspect{L"Aspect", 0.0, -99.0, 99.0, 0.1};
FILTER_ITEM_GROUP a_corrections_end{L""};

FILTER_ITEM_GROUP b_corrections{L"B Corrections"};
FILTER_ITEM_TRACK b_x{L"Position X", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK b_y{L"Position Y", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK b_z{L"Position Z", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK b_cx{L"Center X", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK b_cy{L"Center Y", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK b_cz{L"Center Z", 0.0, -5000.0, 5000.0, 0.1};
FILTER_ITEM_TRACK b_rx{L"Rotation X", 0.0, -3600.0, 3600.0, 0.1};
FILTER_ITEM_TRACK b_ry{L"Rotation Y", 0.0, -3600.0, 3600.0, 0.1};
FILTER_ITEM_TRACK b_rz{L"Rotation Z", 0.0, -3600.0, 3600.0, 0.1};
FILTER_ITEM_TRACK b_scale{L"Scale", 0.0, -99.0, 1000.0, 0.1};
FILTER_ITEM_TRACK b_aspect{L"Aspect", 0.0, -99.0, 99.0, 0.1};
FILTER_ITEM_GROUP b_corrections_end{L""};

void* filter_items[]{
    &progress, &color, &alpha_threshold, &sdf_scale,
    &a_corrections,
    &a_x, &a_y, &a_z, &a_cx, &a_cy, &a_cz,
    &a_rx, &a_ry, &a_rz, &a_scale, &a_aspect,
    &a_corrections_end,
    &b_corrections,
    &b_x, &b_y, &b_z, &b_cx, &b_cy, &b_cz,
    &b_rx, &b_ry, &b_rz, &b_scale, &b_aspect,
    &b_corrections_end,
    nullptr};

struct InstanceState {
  explicit InstanceState(const std::int64_t id) : effect_id(id) {}
  std::int64_t effect_id{};
  PreparationState preparation;
  std::atomic_uint64_t last_warning_generation{
      std::numeric_limits<std::uint64_t>::max()};
};

struct InstanceHolder {
  std::shared_ptr<InstanceState> state;
};

void output_transparent(FILTER_PROC_VIDEO* video) {
  if (video != nullptr && video->set_image_data != nullptr) {
    video->set_image_data(nullptr, 1, 1);
  }
}

void warn_once(const std::shared_ptr<InstanceState>& state, const wchar_t* message) {
  const auto generation = edit_generation.load(std::memory_order_relaxed);
  auto expected = state->last_warning_generation.load(std::memory_order_relaxed);
  if (expected == generation ||
      !state->last_warning_generation.compare_exchange_strong(expected, generation)) {
    return;
  }
  std::wstring text = L"MorphBridge: ";
  text += message;
  text += L"\n";
  OutputDebugStringW(text.c_str());
}

ObjectSpan span_for(EDIT_SECTION& edit, OBJECT_HANDLE object, const ObjectSpan& bridge) {
  const auto frame = edit.get_object_layer_frame(object);
  std::int64_t token = reinterpret_cast<std::intptr_t>(object);
  if (frame.layer == bridge.layer && frame.start == bridge.start && frame.end == bridge.end) {
    token = bridge.token;
  }
  return {token, frame.layer, frame.start, frame.end};
}

std::optional<EndpointPair> find_endpoints(FILTER_PROC_VIDEO& video) {
  if (video.edit == nullptr || video.object == nullptr ||
      video.edit->find_object == nullptr ||
      video.edit->get_object_layer_frame == nullptr) {
    return std::nullopt;
  }
  const ObjectSpan bridge{
      std::numeric_limits<std::int64_t>::min(), video.object->layer,
      video.object->frame_s, video.object->frame_e};
  return resolve_neighbors(bridge, [&](const int layer, const int frame) {
    const auto object = video.edit->find_object(layer, frame);
    if (object == nullptr) {
      return std::optional<ObjectSpan>{};
    }
    return std::optional<ObjectSpan>{span_for(*video.edit, object, bridge)};
  });
}

StandardTransform from_sdk_transform(const OBJECT_IMAGE_PARAM& input) {
  const double sx = std::abs(input.sx);
  const double sy = std::abs(input.sy);
  const double sum = sx + sy;
  return {
      input.x, input.y, input.z,
      input.cx, input.cy, input.cz,
      input.rx, input.ry, input.rz,
      sum * 50.0,
      sum > 0.0 ? (sx - sy) / sum * 100.0 : 0.0,
      input.alpha * 100.0};
}

TransformCorrection a_correction() {
  return {a_x.value, a_y.value, a_z.value,
          a_cx.value, a_cy.value, a_cz.value,
          a_rx.value, a_ry.value, a_rz.value,
          a_scale.value, a_aspect.value};
}

TransformCorrection b_correction() {
  return {b_x.value, b_y.value, b_z.value,
          b_cx.value, b_cy.value, b_cz.value,
          b_rx.value, b_ry.value, b_rz.value,
          b_scale.value, b_aspect.value};
}

void apply_output_transform(
    FILTER_PROC_VIDEO& video,
    OBJECT_HANDLE before,
    OBJECT_HANDLE after,
    const EndpointPair& pair,
    const double amount) {
  if (video.param == nullptr || video.get_output_image_param == nullptr ||
      video.scene == nullptr || video.object == nullptr || video.scene->rate == 0) {
    return;
  }
  const int current_frame = video.object->frame_s + video.object->frame;
  const double seconds_per_frame =
      static_cast<double>(video.scene->scale) / video.scene->rate;
  OBJECT_IMAGE_PARAM before_param{};
  OBJECT_IMAGE_PARAM after_param{};
  if (!video.get_output_image_param(
          before, (pair.before_frame - current_frame) * seconds_per_frame,
          &before_param, sizeof(before_param)) ||
      !video.get_output_image_param(
          after, (pair.after_frame - current_frame) * seconds_per_frame,
          &after_param, sizeof(after_param))) {
    return;
  }
  const auto output = interpolate_corrected_transform(
      from_sdk_transform(before_param), a_correction(),
      from_sdk_transform(after_param), b_correction(), amount);
  const auto factors = scale_factors(output.scale, output.aspect);
  video.param->x = static_cast<float>(output.x);
  video.param->y = static_cast<float>(output.y);
  video.param->z = static_cast<float>(output.z);
  video.param->cx = static_cast<float>(output.cx);
  video.param->cy = static_cast<float>(output.cy);
  video.param->cz = static_cast<float>(output.cz);
  video.param->rx = static_cast<float>(output.rx);
  video.param->ry = static_cast<float>(output.ry);
  video.param->rz = static_cast<float>(output.rz);
  video.param->sx = static_cast<float>(factors.x);
  video.param->sy = static_cast<float>(factors.y);
  video.param->sz = static_cast<float>(std::max(output.scale / 100.0, 1.0e-6));
  video.param->alpha = static_cast<float>(std::clamp(output.opacity / 100.0, 0.0, 1.0));
}

bool process_video(FILTER_PROC_VIDEO* video) {
  if (video == nullptr || video->userdata == nullptr || video->scene == nullptr ||
      video->object == nullptr) {
    output_transparent(video);
    return true;
  }
  auto* holder = static_cast<InstanceHolder*>(video->userdata);
  const auto state = holder->state;
  const auto pair = find_endpoints(*video);
  if (!pair || video->edit->get_object_alias == nullptr) {
    warn_once(state, L"adjacent A/B objects were not found");
    output_transparent(video);
    return true;
  }

  OBJECT_HANDLE before = video->edit->find_object(pair->before.layer, pair->before_frame);
  OBJECT_HANDLE after = video->edit->find_object(pair->after.layer, pair->after_frame);
  if (before == nullptr || after == nullptr) {
    output_transparent(video);
    return true;
  }
  const char* before_alias = video->edit->get_object_alias(before);
  const char* after_alias = video->edit->get_object_alias(after);
  if (before_alias == nullptr || after_alias == nullptr) {
    warn_once(state, L"an endpoint alias could not be read");
    output_transparent(video);
    return true;
  }
  EndpointDescriptor before_descriptor{pair->before, pair->before_frame, before_alias};
  EndpointDescriptor after_descriptor{pair->after, pair->after_frame, after_alias};
  const int scene_id = video->edit->info != nullptr ? video->edit->info->scene_id : 0;
  const int threshold_value = static_cast<int>(std::lround(alpha_threshold.value));
  const auto signature = make_signature(
      scene_id, video->scene->width, video->scene->height,
      before_descriptor, after_descriptor, threshold_value, sdf_scale.value, 1);
  const auto observation = state->preparation.observe(
      edit_generation.load(std::memory_order_relaxed), signature);

  if (observation.action == CacheAction::StartCapture) {
    if (edit_handle == nullptr || edit_handle->rendering_object_video == nullptr ||
        (edit_handle->get_edit_state != nullptr &&
         edit_handle->get_edit_state() == EDIT_HANDLE::EDIT_STATE_SAVE)) {
      state->preparation.fail(observation.request_id, signature);
    } else {
      EndpointCapture capture{[](void* object, int frame, bool apply_effect, void* param,
                                 RenderingVideoCallback callback) {
        return edit_handle != nullptr && edit_handle->rendering_object_video != nullptr &&
               edit_handle->rendering_object_video(
                   object, frame, apply_effect, param, callback);
      }};
      capture.request(
          before, pair->before_frame, after, pair->after_frame,
          observation.request_id, signature,
          [state](CaptureResult result) {
            if (result.images) {
              (void)state->preparation.complete(
                  result.request_id, result.signature, std::move(*result.images));
            } else {
              state->preparation.fail(result.request_id, result.signature);
            }
          });
    }
  }

  const double amount = std::clamp(progress.value / 100.0, 0.0, 1.0);
  apply_output_transform(*video, before, after, *pair, amount);
  const auto prepared = state->preparation.ready();
  if (!prepared) {
    output_transparent(video);
    return true;
  }
  const auto canvas = make_canvas_layout(
      prepared->before.width, prepared->before.height,
      prepared->after.width, prepared->after.height,
      sdf_scale.value, 4);
  if (!canvas) {
    warn_once(state, L"the SDF canvas could not be created");
    output_transparent(video);
    return true;
  }
  GpuRenderRequest request;
  request.video = video;
  request.endpoints = prepared.get();
  request.layout = *canvas;
  request.effect_id = static_cast<std::uint64_t>(state->effect_id);
  request.signature = signature;
  request.progress = static_cast<float>(amount);
  request.alpha_threshold = static_cast<float>(threshold_value / 100.0);
  request.color = {
      color.value.r / 255.0F,
      color.value.g / 255.0F,
      color.value.b / 255.0F,
      1.0F};
  const auto rendered = GpuRenderer{}.render(request);
  if (rendered.error != RenderError::None) {
    warn_once(state, L"GPU rendering failed");
    output_transparent(video);
  }
  return true;
}

void* create_instance(const std::int64_t effect_id) {
  try {
    return new InstanceHolder{std::make_shared<InstanceState>(effect_id)};
  } catch (...) {
    return nullptr;
  }
}

void destroy_instance(std::int64_t, void* userdata) {
  delete static_cast<InstanceHolder*>(userdata);
}

FILTER_PLUGIN_TABLE table{
    FILTER_PLUGIN_TABLE::FLAG_VIDEO |
        FILTER_PLUGIN_TABLE::FLAG_INPUT |
        FILTER_PLUGIN_TABLE::FLAG_USERDATA,
    L"MorphBridge",
    nullptr,
    L"SDF silhouette morph between adjacent objects",
    filter_items,
    &process_video,
    nullptr,
    &create_instance,
    &destroy_instance};

void on_edit_event(void*) {
  edit_generation.fetch_add(1, std::memory_order_relaxed);
}

void on_clear_cache(EDIT_SECTION*) {
  edit_generation.fetch_add(1, std::memory_order_relaxed);
  clear_generation.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace

FILTER_PLUGIN_TABLE* filter_plugin_table() {
  return &table;
}

bool register_morph_bridge(HOST_APP_TABLE* host) {
  if (host == nullptr || host->register_filter_plugin == nullptr ||
      host->create_edit_handle == nullptr ||
      host->register_event_listener == nullptr ||
      host->register_clear_cache_handler == nullptr) {
    return false;
  }
  edit_handle = host->create_edit_handle();
  if (edit_handle == nullptr) {
    return false;
  }
  host->register_filter_plugin(&table);
  host->register_event_listener(EVENT_TYPE::UPDATE_OBJECT, nullptr, &on_edit_event);
  host->register_event_listener(EVENT_TYPE::CHANGE_EDIT_SCENE, nullptr, &on_edit_event);
  host->register_clear_cache_handler(&on_clear_cache);
  return true;
}

std::uint32_t required_version() {
  return 2'010'800;
}

}  // namespace morph_bridge

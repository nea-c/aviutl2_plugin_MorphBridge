#include "cache/cache_state.hpp"

#include <utility>

namespace morph_bridge {

CacheObservation PreparationState::observe(
    const std::uint64_t global_generation,
    const EndpointSignature signature) {
  std::scoped_lock lock{mutex_};
  desired_generation_ = global_generation;
  desired_signature_ = signature;

  if (active_request_id_ != 0) {
    return {CacheAction::WaitForCapture, active_request_id_};
  }

  if (prepared_ && ready_signature_ == signature &&
      ready_generation_ == global_generation) {
    status_ = CacheStatus::CpuReady;
    return {CacheAction::Keep, 0};
  }

  active_request_id_ = ++next_request_id_;
  active_generation_ = global_generation;
  active_signature_ = signature;
  status_ = CacheStatus::Capturing;
  return {CacheAction::StartCapture, active_request_id_};
}

bool PreparationState::complete(
    const std::uint64_t request_id,
    const EndpointSignature signature,
    PreparedEndpoints images) {
  std::scoped_lock lock{mutex_};
  if (request_id == 0 || request_id != active_request_id_ ||
      !active_signature_ || *active_signature_ != signature) {
    return false;
  }

  const auto completed_generation = active_generation_;
  active_request_id_ = 0;
  active_generation_ = 0;
  active_signature_.reset();
  if (desired_generation_ != completed_generation ||
      !desired_signature_ || *desired_signature_ != signature) {
    status_ = CacheStatus::Dirty;
    return false;
  }

  prepared_ = std::make_shared<const PreparedEndpoints>(std::move(images));
  ready_generation_ = completed_generation;
  ready_signature_ = signature;
  status_ = CacheStatus::CpuReady;
  return true;
}

void PreparationState::fail(
    const std::uint64_t request_id, const EndpointSignature signature) {
  std::scoped_lock lock{mutex_};
  if (request_id != active_request_id_ || !active_signature_ ||
      *active_signature_ != signature) {
    return;
  }
  active_request_id_ = 0;
  active_generation_ = 0;
  active_signature_.reset();
  status_ = CacheStatus::Error;
}

std::uint64_t PreparationState::active_request() const {
  std::scoped_lock lock{mutex_};
  return active_request_id_;
}

std::shared_ptr<const PreparedEndpoints> PreparationState::ready() const {
  std::scoped_lock lock{mutex_};
  return prepared_;
}

CacheStatus PreparationState::status() const {
  std::scoped_lock lock{mutex_};
  return status_;
}

}  // namespace morph_bridge

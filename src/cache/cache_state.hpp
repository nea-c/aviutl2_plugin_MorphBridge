#pragma once

#include "cache/signature.hpp"
#include "domain/types.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

namespace morph_bridge {

struct PreparedEndpoints {
  RgbaImage before;
  RgbaImage after;
};

enum class CacheAction { Keep, StartCapture, WaitForCapture };

struct CacheObservation {
  CacheAction action{CacheAction::Keep};
  std::uint64_t request_id{};
};

class PreparationState {
 public:
  [[nodiscard]] CacheObservation observe(
      std::uint64_t global_generation, EndpointSignature signature);

  [[nodiscard]] bool complete(
      std::uint64_t request_id,
      EndpointSignature signature,
      PreparedEndpoints images);

  [[nodiscard]] std::uint64_t active_request() const;
  [[nodiscard]] std::shared_ptr<const PreparedEndpoints> ready() const;
  [[nodiscard]] CacheStatus status() const;

 private:
  mutable std::mutex mutex_;
  std::uint64_t observed_generation_{};
  std::uint64_t next_request_id_{};
  std::uint64_t active_request_id_{};
  std::optional<EndpointSignature> active_signature_;
  std::optional<EndpointSignature> desired_signature_;
  std::optional<EndpointSignature> ready_signature_;
  std::shared_ptr<const PreparedEndpoints> prepared_;
  CacheStatus status_{CacheStatus::Empty};
};

}  // namespace morph_bridge

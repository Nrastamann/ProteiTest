#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>
#include <unordered_map>
#include "mncs_ueconnection.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"
namespace mncs {

class BaseStation {
  static constexpr uint64_t kMaxPower{100};
  static constexpr size_t kQueueLength{6};
  static constexpr size_t kConnectionsLimit{8};
  static constexpr size_t kBufferLength{8};

  using connection_id = uint64_t;

 public:
  BaseStation(int64_t x, int64_t radius) : _x(x), _radius(radius)
  {
    if (radius == 0) {
      _radius = 1;
    }
  }
  [[nodiscard]] uint64_t getPower(int64_t pos) const;
  [[nodiscard]] static size_t calculateMessageID(std::string_view str, size_t msisdn);
  [[nodiscard]] size_t getIdx() const { return _idx; }

 private:
  alignas(utility::kCacheLength) utility::Spinlock _lock_queues;
  utility::Buffer<kBufferLength, kConnectionsLimit> _buffer;

  rigtorp::SPSCQueue<utility::MMEMsg> _mmeMsgs{4};
  rigtorp::SPSCQueue<utility::ENodeBEnodeB> _handoverMsgs{4};

  std::unordered_map<connection_id, std::pair<UEConnection*, pr_utils::Timer>> _connections;

  size_t _idx{};
  int64_t _x;
  int64_t _radius;
  bool _shut_down{false};
};
}  // namespace mncs

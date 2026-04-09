#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"
namespace mncs {
struct Spinlock {
  std::atomic<bool> _lock{false};
  void lock()
  {
    for (;;) {
      if (!_lock.exchange(true, std::memory_order_acquire)) {
        return;
      }
      while (_lock.load(std::memory_order_relaxed))
        ;
    }
  }
  bool tryLock() noexcept
  {
    return !_lock.load(std::memory_order_release) &&
           !_lock.exchange(true, std::memory_order_acquire);
  }
  void unlock() { _lock.store(false, std::memory_order_release); }
};
class BaseStation {
  static constexpr uint64_t kMaxPower{100};

 public:
  BaseStation(int64_t x, int64_t radius) : _x(x), _radius(radius)
  {
    if (radius == 0) {
      _radius = 1;
    }
  }

  int remove();

  [[nodiscard]] uint64_t getPower(int64_t pos) const
  {
    return kMaxPower - (std::abs(_x - pos) / _radius);
  }
  [[nodiscard]] size_t getIdx() const { return _idx; }
  void run(std::unordered_map<size_t, BaseStation>& _enodeb_list);
  [[nodiscard]] size_t getHash(std::string_view str, size_t msisdn) const
  {
    return std::hash<size_t>{}(std::hash<std::string_view>{}(str) +
                               std::hash<size_t>{}(msisdn));
  }

  static constexpr size_t kQueueLength{6};
  rigtorp::SPSCQueue<utility::UEMessageData> _receiveQ{kQueueLength};
  rigtorp::SPSCQueue<utility::UEMessageData> _sendQ{kQueueLength};

  rigtorp::SPSCQueue<utility::ENodeBMMERecv> _receiveMME{kQueueLength};
  rigtorp::SPSCQueue<utility::ENodeBMMESend> _sendMME{kQueueLength};

  rigtorp::SPSCQueue<std::variant<utility::ENodeBEnodeB>> _receiveEnodeB{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::ENodeBEnodeB>> _sendEnodeB{kQueueLength};

 private:
  alignas(utility::kCacheLength) std::mutex _lock;
  std::unordered_multimap<size_t, std::tuple<size_t, std::string, pr_utils::Timer>> _buffer;
  size_t _idx{};
  int64_t _x;
  int64_t _radius;
  bool _shutdown;
};
}  // namespace mncs

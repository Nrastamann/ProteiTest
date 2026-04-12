#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <utility>
#include "ue_messages.hpp"

namespace utility {
inline constexpr size_t kMaxSMSLen{480};

template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};

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

class SmsReqNet;
inline constexpr size_t kThreadNum{8};
inline constexpr size_t kCacheLength = {std::hardware_destructive_interference_size};
template <size_t BufferLength, size_t BufferAmount>
class Buffer {
  using buffer_capacity_type = uint8_t;
  using tmsi = uint32_t;
  using connection_id = size_t;
  using status = bool;
  struct BufferData {
    messages::ue::SmsReqNet _sms;
    tmsi _tmsi;
    bool _flag;
  };
  using buffer_item_type = std::vector<BufferData>;
  using buffer = std::pair<connection_id, buffer_item_type>;
  using buffer_container = std::array<buffer, BufferAmount>;

 public:
  using iterator = buffer_container::iterator;
  Buffer()
  {
    buffer_capacity_type i{0};
    while (i != BufferAmount) {
      _buffer_idx.push(i++);
    }
  }
  void removeBufferData(buffer_container::iterator it, size_t sms_id)
  {
    for (auto* it_small = it->begin(); it_small != it->end(); std::advance(it, 1)) {
      if (it_small->_sms._smsid == sms_id) {
        it->erase(it_small);
        return;
      }
    }
  }
  BufferData& findBufferData(buffer_container::iterator it, size_t sms_id)
  {
    for (auto& el : *it) {
      if (el._sms._smsid == sms_id) {
        return el;
      }
    }
    std::unreachable();
  }
  buffer_container::iterator getBuffer()
  {
    if (_buffer_idx.size() == 0) {
      return _buffers.end();
    }
    _buffer_lock.lock();
    size_t idx = _buffer_idx.front();
    _buffer_idx.pop();
    _buffer_lock.unlock();
    return std::next(_buffers.begin(), idx);
  }
  buffer_container::iterator begin() { return _buffers.begin(); };
  buffer_container::iterator end() { return _buffers.end(); };
  bool bufferAvailability()
  {
    {
      _buffer_lock.lock();
      bool ans = _buffer_idx.size() != 0;
      _buffer_lock.unlock();
      return ans;
    }
  }
  void releaseBuffer(buffer_capacity_type idx)
  {
    if (idx >= BufferAmount) {
      return;
    }
    _buffer_lock.lock();
    _buffer_idx.push(idx);
    _buffer_lock.unlock();
  };
  void releaseBuffer(buffer_container::iterator it)
  {
    if (it >= _buffers.end()) {
      return;
    }
    _buffer_lock.lock();
    _buffer_idx.push(it - _buffers.begin());
    _buffer_lock.unlock();
  };

 private:
  alignas(utility::kCacheLength) utility::Spinlock _buffer_lock;
  std::queue<buffer_capacity_type> _buffer_idx;
  buffer_container _buffers;
};

static constexpr size_t kBufferLength{8};
static constexpr size_t kConnectionsLimit{8};

using default_buffer = utility::Buffer<kBufferLength, kConnectionsLimit>;
};  // namespace utility

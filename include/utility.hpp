#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <queue>
#include <string>
#include <variant>

namespace utility {
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

inline constexpr size_t kThreadNum{8};
inline constexpr size_t kCacheLength = {std::hardware_destructive_interference_size};
static constexpr size_t kMaxSMSLen{480};  //max message send len equals 512 byte
template <size_t BufferLength, size_t BufferAmount>
class Buffer {
  using buffer_capacity_type = uint8_t;
  using buffer_item_type = std::array<char, 1>;
  using buffer = std::array<buffer_item_type, BufferLength>;
  using buffer_container = std::array<buffer, BufferAmount>;

 public:
  Buffer()
  {
    buffer_capacity_type i{0};
    while (i != BufferAmount) {
      _buffer_idx.push(i++);
    }
  }
  buffer_container::iterator getBuffer()
  {
    if (_buffer_idx.size() == 0) {
      return _buffers.end();
    }
    _buffer_lock.lock();
    size_t idx = _buffer_idx.front();
    _buffers.pop();
    _buffer_lock.unlock();
    return std::next(_buffers.begin(), idx);
  }
  buffer_container::iterator end() { return _buffers.end(); };
  bool bufferAvailability() { return _buffer_idx.size() != 0; }
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

//sms req/send to send across network
struct SmsReqNet {
  uint32_t _tmsi;
  uint64_t _msisdn;
  size_t _smsid;
  std::array<char, kMaxSMSLen> _sms;
};

enum class MessageFlag : uint8_t {
  SMSSend,            //Send/receive text
  SMSStatus,          //send/receive Status
  MeasureRequest,     //Request enodeb power
  MeasureResponse,    //respone enodeb power
  AttachRequest,      //Attach req
  AttachResponse,     //Attach resp
  AuthRequest,        //Auth req
  AuthResponse,       //AuthResp
  MeasurementReport,  //reconnect to more powerfull enodeb
  ConfigurationResp,  //Configuration resp
};

enum class SMSStatus : uint8_t {
  Lost,
  Waiting,
  Received,
};
struct SmsUe {
  uint64_t _msisdn;
  std::string _sms;
};
//sms status
struct AcknowledgmentResponse {
  uint32_t _tmsi;
  size_t _message_id;
  SMSStatus _status;
};

//tmsi_d status to mme
struct AcknowledgmentRequest {
  size_t _smsid;
  uint32_t _tmsi_d;
};

//measurement for enodeb power send
struct MeasurementRequest {
  uint64_t _imei;
  int64_t _x;
};

struct EnodeBInfo {
  uint64_t _enodeb_id;
  uint64_t _enodeb_power;
};

//result off measurementreq
//for future use
struct MeasurementResponse {
  uint64_t _imei;
  EnodeBInfo _info;
};

//picked enodeb connect_to
struct MeasurementReport {
  uint64_t _enodeb_idx;
};
enum class EnodeBStatus : uint8_t { CONNECT, DISCONNECT };
//config message
struct ConfigResponse {
  uint64_t _imei;
  uint64_t _ttl;
  EnodeBStatus _status;
};

//config message
struct AttachRequest {
  uint64_t _imei;
  uint64_t _imsi;
  uint64_t _msisdn;
};

//tmsi
struct AttachResponse {};

//config message
struct AuthRequest {
  uint32_t _tmsi;
};

//attached done, correctly
struct AuthResponse {
  uint64_t _imei;
  uint32_t _tmsi;
};

inline constexpr size_t kMsgDataSize{sizeof(SmsReqNet)};

using UEMessageData =
    std::variant<SmsReqNet, AcknowledgmentResponse, AcknowledgmentRequest, MeasurementRequest,
                 MeasurementResponse, MeasurementReport, AttachRequest, AttachResponse,
                 AuthRequest, AuthResponse, ConfigResponse, std::array<char, kMsgDataSize>>;

struct UEMessage {
  MessageFlag _msg_type;
  UEMessageData _data;
};

inline constexpr size_t kMsgSize{sizeof(UEMessage)};
};  // namespace utility

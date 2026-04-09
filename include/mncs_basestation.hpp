#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <unordered_map>
#include <vector>
#include "exchange_ue.hpp"
#include "mncs_listener.hpp"
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
  void unlock() { _lock.store(false, std::memory_order_release); }
};
class EnodeBStorage {};
class BaseStation {
  static constexpr size_t kQueueLength{4};
  static constexpr uint64_t kMaxPower{100};
  BaseStation(int64_t x, int64_t radius) : _x(x), _radius(radius)
  {
    if (radius == 0) {
      _radius = 1;
    }
  }
  using recv_map = std::unordered_map<utility::MessageFlag,
                                      std::function<void(utility::ENodeBMessageData&)>>;
  static const recv_map& getMap()
  {
    const static recv_map receive_work{
        {utility::MessageFlag::SMSSend,
         [](utility::UEMessageData& message) {
           message = *reinterpret_cast<utility::SmsReqNet*>(
               &std::get<std::array<char, utility::kMsgDataSize>>(message));
           if constexpr (std::endian::native == std::endian::little) {
             auto& msg = std::get<utility::SmsReqNet>(message);
             msg._msisdn = std::byteswap(msg._msisdn);
             msg._smsid = std::byteswap(msg._smsid);
             msg._tmsi = std::byteswap(msg._tmsi);
           }
         }},
        {utility::MessageFlag::SMSStatus,
         [](utility::UEMessageData& message) {
           message = *reinterpret_cast<utility::AcknowledgmentResp*>(
               &std::get<std::array<char, utility::kMsgDataSize>>(message));
           if constexpr (std::endian::native == std::endian::little) {
             auto& msg = std::get<utility::AcknowledgmentResp>(message);
             msg._message_id = std::byteswap(msg._message_id);
             msg._tmsi = std::byteswap(msg._tmsi);
           }
         }},  //function to set status
        {utility::MessageFlag::AuthResp,
         [](utility::UEMessageData& message) {
           message = *reinterpret_cast<utility::AuthResp*>(
               &std::get<std::array<char, utility::kMsgDataSize>>(message));
         }},
        {utility::MessageFlag::RangeResp,
         [](utility::UEMessageData& message) {
           message = *reinterpret_cast<utility::MeasurementResp*>(
               &std::get<std::array<char, utility::kMsgDataSize>>(message));
           if constexpr (std::endian::native == std::endian::little) {
             auto& msg = std::get<utility::MeasurementResp>(message);
             msg._imei = std::byteswap(msg._imei);
             msg._info._enodeb_id = std::byteswap(msg._info._enodeb_id);
             msg._info._enodeb_power = std::byteswap(msg._info._enodeb_power);
           }
         }},
        {utility::MessageFlag::Configuration,
         [](utility::UEMessageData& message) {
           message = *reinterpret_cast<utility::ConfigResp*>(
               &std::get<std::array<char, utility::kMsgDataSize>>(message));
           if constexpr (std::endian::native == std::endian::little) {
             auto& msg = std::get<utility::ConfigResp>(message);
             msg._imei = std::byteswap(msg._imei);
             msg._ttl = std::byteswap(msg._ttl);
           }
         }},
        {utility::MessageFlag::AttachResp, [](utility::UEMessageData& message) {
           message = *reinterpret_cast<utility::AttachResponse*>(
               &std::get<std::array<char, utility::kMsgDataSize>>(message));
           if constexpr (std::endian::native == std::endian::little) {
             auto& msg = std::get<utility::AttachResponse>(message);
             msg._tmsi = std::byteswap(msg._tmsi);
           }
         }}};
    return receive_work;
  }

 public:
  using flagMap = const std::unordered_map<utility::MessageFlag,
                                           std::function<void(utility::EnodeBMessage&)>>;
  [[nodiscard]] uint64_t getPower(int64_t pos) const
  {
    return kMaxPower - (std::abs(_x - pos) / _radius);
  }

  void connect() {}
  void push()
  {
    if (_buffer_received.size() != 0) {}
  }
  void run();

 private:
  std::array<rigtorp::template SPSCQueue<utility::ENodeBMessageData>, kThreadNum> _read_queue{
      {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength},
      {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength}};
  std::array<rigtorp::template SPSCQueue<utility::UEMessageData>, kThreadNum> _buffer_received{
      {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength},
      {kQueueLength}, {kQueueLength}, {kQueueLength}, {kQueueLength}};
  std::array<bool, kThreadNum> _flags{false};
  std::array<Spinlock, kThreadNum> _locks;
  std::array<size_t, kThreadNum> _counters;
  std::array<std::vector<utility::SmsReqNet>, kThreadNum> _buffers;

  const recv_map& _map = getMap();

  int64_t _x;
  int64_t _radius;
  bool _shutdown;
};
}  // namespace mncs

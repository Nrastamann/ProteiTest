#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <unordered_map>
#include <variant>
#include <vector>
#include "exchange_ue.hpp"
#include "mncs_listener.hpp"
#include "mncs_ueconnection.hpp"
#include "utility.hpp"

namespace mncs {
class UEConnection;
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
  using flagMap = const std::unordered_map<utility::MessageFlag,
                                           std::function<void(utility::EnodeBMessage&)>>;

 public:
  BaseStation(int64_t x, int64_t radius) : _x(x), _radius(radius)
  {
    if (radius == 0) {
      _radius = 1;
    }
  }

  int remove();

  BaseStation* connect(UEConnection& connection);

  [[nodiscard]] uint64_t getPower(int64_t pos) const
  {
    return kMaxPower - (std::abs(_x - pos) / _radius);
  }
  [[nodiscard]] size_t getIdx() const { return _idx; }
  void run()
  {
    while (!_shutdown) {
      if (auto* msg = _receiveQ.front(); msg != nullptr) {
        std::visit(
            utility::Visitor{
                [this](utility::SmsReqNet&& msg) {
                  buffer.push_back(
                      std::pair{std::move(msg._sms), getHash(msg._sms, msg._msisdn)});
                  _counter++;
                  _sendMME.push(utility::SMSRoute{._msisdn = msg._msisdn, ._tmsi = msg._tmsi});
                },
                [this](utility::AcknowledgmentReq&& msg) {
                  _sendMME.push(
                      utility::DeliveryReport{._sms_id = msg._smsid,
                                              ._tmsi_id = msg._tmsi_d,
                                              ._status = utility::SMSStatus::Received});
                },
                [this](utility::MeasurementReq&& msg) {
                  size_t power = getPower(msg._x);
                  _sendQ.push(utility::MeasurementResp{
                      ._imei = msg._imei, ._info{._enodeb_id = _idx, ._enodeb_power = power}});
                },

                [this](utility::MeasurementConnectReq&& msg) {
                  _sendMME.push(utility::HandoverReq{._enodeb_id = msg._enodeb_idx});
                },

                [this](utility::AttachReq&& msg) {
                  _sendMME.push(utility::EnodeBIDSend{._id = _idx});
                },
                [](auto&& msg) {},
            },
            std::move(*msg));
      }
      if (auto* msg = _receiveMME.front(); msg != nullptr) {
        std::visit(utility::Visitor{
                       [](utility::AttachReqMME&& msg) {},
                       [](utility::SwitchReq&& msg) {},
                       [](utility::EnodeBIDSend&& msg) {},
                       [](utility::TimeoutUE&& msg) {},
                       [](utility::ResetSmsttl&& msg) {},
                       [](utility::DeliveryReport&& msg) {},
                       [](utility::StatusReport&& msg) {},
                       [](utility::SMSRoute&& msg) {},
                       [](utility::updateLocation&& msg) {},
                       [](auto&& msg) {},
                   },
                   std::move(*msg));
      }
    }
  }
  [[nodiscard]] size_t getHash(std::string_view str, size_t msisdn) const
  {
    return std::hash<size_t>{}(std::hash<std::string_view>{}(str) +
                               std::hash<size_t>{}(_counter + msisdn));
  }

  static constexpr size_t kQueueLength{6};
  rigtorp::SPSCQueue<utility::UEMessageData> _receiveQ{kQueueLength};
  rigtorp::SPSCQueue<utility::UEMessageData> _sendQ{kQueueLength};

  rigtorp::SPSCQueue<utility::ENodeBMMERecv> _receiveMME{kQueueLength};
  rigtorp::SPSCQueue<utility::ENodeBMMESend> _sendMME{kQueueLength};

  rigtorp::SPSCQueue<std::variant<>> _receiveEnodeB{kQueueLength};
  rigtorp::SPSCQueue<std::variant<>> _sendEnodeB{kQueueLength};

 private:
  std::mutex _mtx_handover;
  std::vector<std::pair<std::string, size_t>> buffer;
  size_t _counter{0};
  size_t _idx{};
  int64_t _x;
  int64_t _radius;
  bool _shutdown;
};
}  // namespace mncs

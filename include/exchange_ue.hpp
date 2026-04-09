#pragma once
#include <bits/chrono.h>
#include <unistd.h>
#include <atomic>
#include <bit>
#include <chrono>
#include <functional>
#include <thread>
#include <unordered_map>
#include <variant>
#include "data_pool.hpp"
#include "ip_addr.hpp"
#include "rigtorp/SPSCQueue.h"
#include "ue_context.hpp"
#include "utility.hpp"
namespace ue {
using recv_map =
    std::unordered_map<utility::MessageFlag, std::function<void(utility::UEMessageData&)>>;
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

enum class ConnectionStatus : uint8_t { socket_creation_err, connection_err, valid_connection };
class Exchanger {
  static constexpr std::chrono::milliseconds kSleepTime{50};
  static constexpr size_t kSendQueueNumber{4};
  static constexpr size_t kQueueLength{15};

 public:
  Exchanger(DeviceConfiguration& ctxt, network_addr::IpAddr& addr, int64_t x)
      : _ctxt(ctxt, 0, 0), _ip_addr(addr), _x(x) {};
  Exchanger(ue::UeContext& ctxt, network_addr::IpAddr& addr, int64_t x)
      : _ctxt(ctxt), _ip_addr(addr), _x(x) {};

  void attachTask(rigtorp::SPSCQueue<size_t>& indexes_to_remove);
  void pingTask();
  void sendTask();
  void receiveTask();

  void receiveSmsTask(data_storage::DataPool& data)
  {
    while (_in_active) {
      if (_receivedSmsQ.size() == 0) {
        std::this_thread::sleep_for(kSleepTime);
        continue;
      }
      auto* sms = _receivedSmsQ.front();
      data.pushSms(std::move(*sms));
      _receivedSmsQ.pop();
    }
  }
  void receiveSmsStatusTask(data_storage::DataPool& data)
  {

    while (_in_active) {
      if (_receivedSmsStatusQ.size() == 0) {
        std::this_thread::sleep_for(kSleepTime);
        continue;
      }
      auto* sms = _receivedSmsStatusQ.front();
      data.pushStatus(std::move(*sms));
      _receivedSmsStatusQ.pop();
    }
  }

  void setActive(data_storage::DataPool& DataPool)
  {
    _in_active = !_in_active;

    if (_in_active) {  //need to understand, what of these need to restart
      createSocket();
      std::thread worker1(&Exchanger::pingTask, this);
      std::thread worker2(&Exchanger::receiveSmsTask, this, std::ref(DataPool));
      std::thread worker3(&Exchanger::receiveSmsStatusTask, this, std::ref(DataPool));
      std::thread worker4(&Exchanger::sendTask, this);
      std::thread worker5(&Exchanger::receiveTask, this);

      worker1.detach();
      worker2.detach();
      worker3.detach();
      worker4.detach();
      worker5.detach();
    }
    else {
      closeConnection();
    }
  }
  [[nodiscard]] network_addr::IpAddr& getAddr() { return _ip_addr; }
  [[nodiscard]] ue::UeContext& getContext() { return _ctxt; }
  [[nodiscard]] double power() const { return _power; }
  [[nodiscard]] size_t enodeb() const { return _picked_enodeb; }
  [[nodiscard]] int socket() const { return _socket; }
  [[nodiscard]] bool inActive() const { return _in_active; }
  [[nodiscard]] bool attached() const { return _attached; }
  [[nodiscard]] int64_t x() const { return _x; }
  void moveX(int64_t delta) { _x += delta; }

  void pushToSend(
      std::variant<std::variant<utility::MeasurementReq, utility::MeasurementConnectReq>,
                   std::string, utility::AcknowledgmentReq, utility::AttachReq,
                   utility::AuthReq>&& msg)
  {
    std::visit(
        utility::Visitor{
            [this](std::variant<utility::MeasurementReq, utility::MeasurementConnectReq>&&
                       measurement) { _poolingSendQ.push(std::move(measurement)); },
            [this](std::string&& str) { _sendSmsQ.push(std::move(str)); },
            [this](auto&& attach) {
              _attachment_send_q.push(std::forward<decltype(attach)>(attach));
            },
            [this](utility::AcknowledgmentReq&& ack) {
              _acknowledgementQ.push(std::move(ack));
            },
        },
        std::move(msg));
  }

  void pushToRecv(
      std::variant<
          utility::SmsReqNet, std::variant<utility::ConfigResp, utility::MeasurementResp>,
          utility::AcknowledgmentResp, utility::AttachResponse, utility::AuthResp>&& msg)
  {
    std::visit(
        utility::Visitor{
            [this](std::variant<utility::ConfigResp, utility::MeasurementResp>&& measurement) {
              _poolingBufferQ.push(std::move(measurement));
            },
            [this](utility::SmsReqNet&& str) { _receivedSmsQ.push(std::move(str)); },
            [this](auto&& attach) {
              _attachment_recv_q.push(std::forward<decltype(attach)>(std::move(attach)));
            },
            [this](utility::AcknowledgmentResp&& ack) {
              _receivedSmsStatusQ.push(std::move(ack));
            },
        },
        std::move(msg));
  }
  void closeConnection()
  {
    close(_socket);
    _attached = false;
    _in_active = false;
  }

 private:
  ConnectionStatus createSocket();
  static constexpr size_t kAttachmentQLen{2};
  alignas(utility::kCacheLength) std::atomic<bool> _in_active{false};
  alignas(utility::kCacheLength) std::atomic<bool> _attached{false};
  alignas(utility::kCacheLength) std::atomic<bool> _attachment_in_process{false};

  //send queues
  rigtorp::SPSCQueue<utility::AcknowledgmentReq> _acknowledgementQ{kQueueLength};
  rigtorp::SPSCQueue<std::string> _sendSmsQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::MeasurementReq, utility::MeasurementConnectReq>>
      _poolingSendQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::AttachReq, utility::AuthReq>> _attachment_send_q{
      kAttachmentQLen};

  //receive queues
  rigtorp::SPSCQueue<utility::SmsReqNet> _receivedSmsQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::ConfigResp, utility::MeasurementResp>>
      _poolingBufferQ{kQueueLength};
  rigtorp::SPSCQueue<utility::AcknowledgmentResp> _receivedSmsStatusQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::AttachResponse, utility::AuthResp>>
      _attachment_recv_q{kAttachmentQLen};

  UeContext _ctxt;

  const recv_map& _receive_map = getMap();

  network_addr::IpAddr _ip_addr;

  //std::counting_semaphore<kSendQueueNumber>{0};
  //std::counting_semaphore<kSendQueueNumber> _send_queue_sm{0};
  double _power{};
  size_t _picked_enodeb{0};
  int64_t _x;
  int _socket{-1};
};
}  // namespace ue

#pragma once
#include <unistd.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <semaphore>
#include <thread>
#include <unordered_map>
#include <variant>
#include "ip_addr.hpp"
#include "rigtorp/SPSCQueue.h"
#include "ue_context.hpp"
#include "utility.hpp"
namespace ue {
enum class ConnectionStatus : uint8_t { socket_creation_err, connection_err, valid_connection };
class Exchanger {
  static constexpr size_t kSendQueueNumber{4};
  static constexpr size_t kQueueLength{15};

 public:
  void attachTask();
  void pingTask();
  void receiveSmsTask();
  void receiveSmsStatusTask();
  void sendTask();
  void receiveTask();

  void setActive(bool value)
  {
    _in_active = value;
    if (_in_active) {  //need to understand, what of these need to restart
      std::thread worker1(&Exchanger::attachTask, this);
      std::thread worker2(&Exchanger::pingTask, this);
      std::thread worker3(&Exchanger::receiveSmsTask, this);
      std::thread worker4(&Exchanger::receiveSmsStatusTask, this);
      std::thread worker5(&Exchanger::sendTask, this);
      std::thread worker6(&Exchanger::receiveTask, this);

      worker1.detach();
      worker2.detach();
      worker3.detach();
      worker4.detach();
      worker5.detach();
      worker6.detach();
    }
  }

 private:
  void closeConnection()
  {
    close(_socket);
    _attached = false;
    _in_active = false;
  }
  ConnectionStatus createSocket();
  using recv_map =
      std::unordered_map<utility::MessageFlag, std::function<void(utility::UEMessageData&&)>>;

  const recv_map& getMap()
  {
    const static recv_map receive_work{
        {utility::MessageFlag::SMSSend,
         [this](utility::UEMessageData&& message) {
           this->_receivedSmsQ.push(std::get<utility::SmsReqNet>(std::move(message)));
         }},
        {utility::MessageFlag::SMSStatus,
         [this](utility::UEMessageData&& message) {
           _receivedSmsStatusQ.push(std::get<utility::AcknowledgmentReq>(std::move(message)));
         }},  //function to set status
        {utility::MessageFlag::AuthResp,
         [this](utility::UEMessageData&& message) {
           _attachment_recv_q.push(std::get<utility::AttachResult>(std::move(message)));
         }},
        {utility::MessageFlag::RangeResp,
         [this](utility::UEMessageData&& message) {
           this->_poolingBufferQ.push(
               std::get<utility::MeasurementControl>(std::move(message)));
         }},
        {utility::MessageFlag::Configuration,
         [this](utility::UEMessageData&& message) {
           this->_poolingBufferQ.push(std::get<utility::ConfigReq>(std::move(message)));
         }},
        {utility::MessageFlag::AttachResp, [this](utility::UEMessageData&& message) {
           this->_attachment_recv_q.push(std::get<utility::AttachResponse>(std::move(message)));
         }}};
    return receive_work;
  }

  //send queues
  rigtorp::SPSCQueue<utility::AcknowledgmentUE> _acknowledgementQ{kQueueLength};
  rigtorp::SPSCQueue<std::string> _sendSmsQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::MeasurementReq, utility::MeasurementReport>>
      _poolingSendQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::AttachReq, utility::AuthReq>> _attachment_send_q{
      kQueueLength};

  //receive queues
  rigtorp::SPSCQueue<utility::SmsReqNet> _receivedSmsQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::ConfigReq, utility::MeasurementControl>>
      _poolingBufferQ{kQueueLength};
  rigtorp::SPSCQueue<utility::AcknowledgmentReq> _receivedSmsStatusQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<utility::AttachResponse, utility::AttachResult>>
      _attachment_recv_q{kQueueLength};

  UeContext _ctxt;

  const recv_map& _receive_map = getMap();

  network_addr::IpAddr _ip_addr;

  std::counting_semaphore<kSendQueueNumber> _send_queue_sm{0};
  std::counting_semaphore<2> _attach_sm{1};

  //std::counting_semaphore<kSendQueueNumber>{0};
  //std::counting_semaphore<kSendQueueNumber> _send_queue_sm{0};

  size_t _picked_enodeb{0};
  int _socket;
  std::atomic<bool> _in_active;
  std::atomic<bool> _attached{false};
};
}  // namespace ue

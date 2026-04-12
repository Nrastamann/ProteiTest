#pragma once
#include <bits/chrono.h>
#include <unistd.h>
#include <atomic>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <variant>
#include "data_pool.hpp"
#include "ip_addr.hpp"
#include "rigtorp/SPSCQueue.h"
#include "ue_context.hpp"
#include "ue_messages.hpp"
#include "utility.hpp"
namespace ue {
using net_conversion_map =
    std::unordered_map<messages::ue::MessageFlag,
                       std::function<void(messages::ue::UEMessageData&)>>;
const net_conversion_map& getMap();

enum class ConnectionStatus : uint8_t { socket_creation_err, connection_err, valid_connection };
class Exchanger {
  static constexpr std::chrono::milliseconds kSleepTime{50};
  static constexpr size_t kSendQueueNumber{4};
  static constexpr size_t kQueueLength{15};
  using send_type = std::variant<
      std::variant<messages::ue::MeasurementRequest, messages::ue::MeasurementReport>,
      std::string, messages::ue::AcknowledgmentRequest, messages::ue::AttachRequest,
      messages::ue::AuthResponse>;
  using recv_type = std::variant<
      messages::ue::SmsReqNet,
      std::variant<messages::ue::ConfigResponse, messages::ue::MeasurementResponse>,
      messages::ue::AcknowledgmentResponse, messages::ue::AttachResponse,
      messages::ue::AuthRequest>;

 public:
  Exchanger(DeviceConfiguration& ctxt, network_addr::IpAddr& addr, int64_t x)
      : _ctxt(ctxt, 0, 0), _ip_addr(addr), _x(x) {};
  Exchanger(ue::UeContext& ctxt, network_addr::IpAddr& addr, int64_t x)
      : _ctxt(ctxt), _ip_addr(addr), _x(x) {};

  void attachTask(rigtorp::SPSCQueue<size_t>& indexes_to_remove);
  void pingTask();
  void sendTask();
  void receiveTask();

  void receiveSmsTask(data_storage::DataPool& data);
  void receiveSmsStatusTask(data_storage::DataPool& data);

  void setActive(data_storage::DataPool& DataPool);

  [[nodiscard]] network_addr::IpAddr& getAddr() { return _ip_addr; }
  [[nodiscard]] ue::UeContext& getContext() { return _ctxt; }
  [[nodiscard]] double power() const { return _power; }
  [[nodiscard]] size_t enodeb() const { return _picked_enodeb; }
  [[nodiscard]] int socket() const { return _socket; }
  [[nodiscard]] bool inActive() const { return _in_active; }
  [[nodiscard]] bool attached() const { return _attached; }
  [[nodiscard]] int64_t x() const { return _x; }
  void moveX(int64_t delta) { _x += delta; }

  void pushToSend(send_type&& msg);
  void pushToRecv(recv_type&& msg);

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
  rigtorp::SPSCQueue<messages::ue::AcknowledgmentRequest> _acknowledgementQ{kQueueLength};
  rigtorp::SPSCQueue<std::string> _sendSmsQ{kQueueLength};
  rigtorp::SPSCQueue<
      std::variant<messages::ue::MeasurementRequest, messages::ue::MeasurementReport>>
      _poolingSendQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<messages::ue::AttachRequest, messages::ue::AuthResponse>>
      _attachment_send_q{kAttachmentQLen};

  //receive queues
  rigtorp::SPSCQueue<messages::ue::SmsReqNet> _receivedSmsQ{kQueueLength};
  rigtorp::SPSCQueue<
      std::variant<messages::ue::ConfigResponse, messages::ue::MeasurementResponse>>
      _poolingBufferQ{kQueueLength};
  rigtorp::SPSCQueue<messages::ue::AcknowledgmentResponse> _receivedSmsStatusQ{kQueueLength};
  rigtorp::SPSCQueue<std::variant<messages::ue::AttachResponse, messages::ue::AuthRequest>>
      _attachment_recv_q{kAttachmentQLen};

  UeContext _ctxt;

  const net_conversion_map& _receive_map = getMap();

  network_addr::IpAddr _ip_addr;

  //std::counting_semaphore<kSendQueueNumber>{0};
  //std::counting_semaphore<kSendQueueNumber> _send_queue_sm{0};
  double _power{};
  size_t _picked_enodeb{0};
  int64_t _x;
  int _socket{-1};
};
}  // namespace ue

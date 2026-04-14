#include "exchange_ue.hpp"
#include <byteswap.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <array>
#include <bit>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <iterator>
#include <thread>
#include <unordered_map>
#include <variant>
#include "ip_addr.hpp"
#include "logger.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "ue_messages.hpp"
#include "utility.hpp"

namespace ue {
const net_conversion_map& getMap()
{
  const static net_conversion_map receive_work{
      {messages::ue::MessageFlag::SMSSend,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::SmsReqNet*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::SmsReqNet>(message);
           msg._msisdn = std::byteswap(msg._msisdn);
           msg._tmsi = std::byteswap(msg._tmsi);
           msg._smsid = std::byteswap(msg._smsid);
         }
       }},
      {messages::ue::MessageFlag::SMSStatus,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::AcknowledgmentResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::AcknowledgmentResponse>(message);
           msg._tmsi = std::byteswap(msg._tmsi);
           msg._message_id = std::byteswap(msg._message_id);
         }
       }},  //function to set status
      {messages::ue::MessageFlag::MeasureRequest,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::MeasurementRequest*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::MeasurementRequest>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._x = std::byteswap(msg._x);
         }
       }},
      {messages::ue::MessageFlag::MeasureResponse,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::MeasurementResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::MeasurementResponse>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._info._enodeb_power = std::byteswap(msg._info._enodeb_power);
           msg._info._enodeb_id = std::byteswap(msg._info._enodeb_id);
         }
       }},
      {messages::ue::MessageFlag::AttachRequest,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::AttachRequest*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::AttachRequest>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._msisdn = std::byteswap(msg._msisdn);
           msg._imsi = std::byteswap(msg._imsi);
         }
       }},
      {messages::ue::MessageFlag::AttachResponse,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::AttachResponse*>(&message);
         auto& msg = std::get<messages::ue::AttachResponse>(message);
       }},
      {messages::ue::MessageFlag::MeasurementReport,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::MeasurementReport*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::MeasurementReport>(message);
           msg._enodeb_idx = std::byteswap(msg._enodeb_idx);
         }
       }},
      {messages::ue::MessageFlag::ConfigurationResp,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::ConfigResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::ConfigResponse>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._ttl = std::byteswap(msg._ttl);
         }
       }},
      {messages::ue::MessageFlag::AuthRequest,
       [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::AuthRequest*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::AuthRequest>(message);
           msg._tmsi = std::byteswap(msg._tmsi);
         }
       }},
      {messages::ue::MessageFlag::AuthResponse, [](messages::ue::UEMessageData& message) {
         message = *std::bit_cast<messages::ue::AuthResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<messages::ue::AuthResponse>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._tmsi = std::byteswap(msg._tmsi);
         }
       }}};
  return receive_work;
}
void Exchanger::receiveSmsTask(data_storage::DataPool& data)
{
  while (_in_active) {
    if (_receivedSmsQ.size() == 0) {
      std::this_thread::sleep_for(kSleepTime);
      continue;
    }
    auto* sms = _receivedSmsQ.front();
    data.pushSms(*sms);
    _receivedSmsQ.pop();
  }
}
void Exchanger::receiveSmsStatusTask(data_storage::DataPool& data)
{
  while (_in_active) {
    if (_receivedSmsStatusQ.size() == 0) {
      std::this_thread::sleep_for(kSleepTime);
      continue;
    }
    auto* sms = _receivedSmsStatusQ.front();
    data.pushStatus(*sms);
    _receivedSmsStatusQ.pop();
  }
}

void Exchanger::setActive(data_storage::DataPool& DataPool)
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
void Exchanger::pushToSend(send_type&& msg)
{
  std::visit(
      utility::Visitor{
          [this](std::variant<messages::ue::MeasurementRequest, messages::ue::MeasurementReport>
                     measurement) { _poolingSendQ.push(measurement); },
          [this](std::string&& str) { _sendSmsQ.push(std::move(str)); },
          [this](auto attach) { _attachment_send_q.push(attach); },
          [this](messages::ue::AcknowledgmentRequest ack) { _acknowledgementQ.push(ack); },
      },
      std::move(msg));
}

void Exchanger::pushToRecv(recv_type&& msg)
{
  std::visit(
      utility::Visitor{
          [this](std::variant<messages::ue::ConfigResponse, messages::ue::MeasurementResponse>
                     measurement) { _poolingBufferQ.push(measurement); },
          [this](messages::ue::SmsReqNet str) { _receivedSmsQ.push(str); },
          [this](auto attach) {
            _attachment_recv_q.push(std::forward<decltype(attach)>(attach));
          },
          [this](messages::ue::AcknowledgmentResponse ack) { _receivedSmsStatusQ.push(ack); },
      },
      std::move(msg));
}

void Exchanger::sendTask()
{
  //need to change logger behaviour in multithread mode, need to lock less
  //and run it separate thread
  // logging::MultithreadPresets::functionCall();
  std::array<messages::ue::UEMessage, kSendQueueNumber> packets;
  const auto* it_begin = packets.begin();
  while (_in_active) {

    bool is_ack = _acknowledgementQ.size() != 0;
    bool is_sms = _sendSmsQ.size() != 0;
    bool is_pool = _poolingSendQ.size() != 0;
    bool is_attach = _attachment_send_q.size() != 0;

    if (!(is_ack || is_sms || is_pool || is_attach)) {
      std::this_thread::sleep_for(kSleepTime);
      continue;
    }
    auto* it = packets.begin();
    if (messages::ue::AcknowledgmentRequest* data = _acknowledgementQ.front(); is_ack) {
      messages::ue::UEMessage msg = {._data = *data};

      auto it_func = _receive_map.at(messages::ue::MessageFlag::SMSStatus);

      *it = messages::ue::UEMessage{._msg_type = messages::ue::MessageFlag::SMSStatus,
                                    ._data = msg._data};
      std::advance(it, 1);
      it_func(it->_data);
      _acknowledgementQ.pop();
    }

    if (std::string* data = _sendSmsQ.front(); is_sms && _attached) {

      auto it_func = _receive_map.at(messages::ue::MessageFlag::SMSSend);
      *it = messages::ue::UEMessage{._msg_type = messages::ue::MessageFlag::SMSSend,
                                    ._data = messages::ue::SmsReqNet{._tmsi = _ctxt.tmsi(),
                                                                     ._msisdn = _ctxt.msisdn(),
                                                                     ._smsid = 0,
                                                                     ._sms = {*data->data()}}};
      std::advance(it, 1);
      it_func(it->_data);
      _sendSmsQ.pop();
    }

    if (auto* data = _poolingSendQ.front(); is_pool) {
      std::visit(
          utility::Visitor{
              [&it, this, &packets](messages::ue::MeasurementRequest req) {
                *it = messages::ue::UEMessage{
                    ._msg_type = messages::ue::MessageFlag::MeasureRequest, ._data = req};
                _receive_map.at(messages::ue::MessageFlag::MeasureRequest)(it->_data);
              },
              [&it, this](messages::ue::MeasurementReport req) {
                *it = messages::ue::UEMessage{
                    ._msg_type = messages::ue::MessageFlag::MeasurementReport, ._data = req};
                auto it_func = _receive_map.at(messages::ue::MessageFlag::MeasurementReport);
                it_func(it->_data);
              }},
          *data);
      std::advance(it, 1);
      _poolingSendQ.pop();
    }

    if (auto* data = _attachment_send_q.front(); is_attach) {
      messages::ue::UEMessage msg;
      std::visit(
          utility::Visitor{
              [&it, this, &msg](messages::ue::AttachRequest req) {
                msg._data = req;
                *it = messages::ue::UEMessage{
                    ._msg_type = messages::ue::MessageFlag::AttachRequest, ._data = msg._data};
                auto it_func = _receive_map.at(messages::ue::MessageFlag::AttachRequest);
                it_func(it->_data);
              },
              [&it, this, &msg](messages::ue::AuthResponse req) {
                msg._data = req;
                *it = messages::ue::UEMessage{
                    ._msg_type = messages::ue::MessageFlag::AuthResponse, ._data = msg._data};
                auto it_func = _receive_map.at(messages::ue::MessageFlag::AuthResponse);
                it_func(it->_data);
              }},
          *data);
      std::advance(it, 1);
      _attachment_send_q.pop();
    }
    if (it - it_begin <= 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    std::cout << '\n';
    ssize_t status =
        send(_socket, packets.begin(), (it - it_begin) * sizeof(messages::ue::UEMessage), 0);

    if (status == -1) {
      closeConnection();
      std::cout << "SEND ERROR\n";  //temp
    }
  }
}

void Exchanger::receiveTask()
{
  std::array<messages::ue::UEMessage, kSendQueueNumber> packets;

  while (_in_active) {
    ssize_t received_amount = recv(_socket, packets.begin(), sizeof(packets), 0);
    if (received_amount == 0) {
      continue;
    }
    if (received_amount == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        continue;
      }
      std::cout << "RECEIVE ERROR\n";
      closeConnection();
      break;
    }

    const auto* it_end =
        std::next(packets.end(),
                  -static_cast<int64_t>((received_amount / sizeof(messages::ue::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      auto cast_fn = _receive_map.find(it->_msg_type);
      cast_fn != _receive_map.end() ? cast_fn->second(it->_data) : closeConnection();
      if (!_in_active) {
        break;
      }

      std::visit(utility::Visitor{
                     [this](auto&& msg) { pushToRecv(msg); },
                     [](messages::ue::AcknowledgmentRequest) {},
                     [](messages::ue::MeasurementRequest) {},
                     [](messages::ue::MeasurementReport) {}, [](messages::ue::AttachRequest) {},
                     [](messages::ue::AuthResponse) {}, [](messages::ue::AttachResponse) {},
                     [](std::array<char, messages::ue::kMsgDataSize>) {}},
                 it->_data);
    }
  }
}

ConnectionStatus Exchanger::createSocket()
{
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (_socket == -1) {
    std::format("couldn't create socket to {}", _ip_addr);
    _in_active = false;
    return ConnectionStatus::socket_creation_err;
  }

  sockaddr_in server_addr{.sin_family = AF_INET,
                          .sin_port = htons(_ip_addr._port),
                          .sin_addr{_ip_addr.addrToNetwork()},
                          .sin_zero{0}};
  //NOLINTNEXTLINE
  int res = connect(_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

  if (res != 0) {
    close(_socket);
    std::cout << std::format("couldn't connect to {}\n", _ip_addr);
    return ConnectionStatus::connection_err;
  }
  return ConnectionStatus::valid_connection;
}

static uint64_t findmaxPower(const std::unordered_map<uint64_t, uint64_t>& list)
{
  uint64_t idx = list.begin()->first;
  uint64_t max = list.begin()->second;
  for (const auto& enodeb : list) {
    if (max < enodeb.second) {
      idx = enodeb.first;
      max = enodeb.second;
    }
  }

  return idx;
}
void Exchanger::pingTask()
{
  //map of all received enodeb, pick whenever, pick only if signal power greater than threshold
  std::unordered_map<uint64_t, uint64_t> enodeb_list;
  uint64_t max_enodeb_idx{};
  uint64_t max_power{};
  bool retryattach = false;
  pr_utils::Timer timer_ping(500);
  pushToSend(messages::ue::MeasurementRequest{._imei = _ctxt.imei(), ._x = this->_x});
  static constexpr double kTtlcoef{1.25};
  while (_in_active) {
    auto* pooling = _poolingBufferQ.front();
    while (pooling != nullptr) {
      std::visit(
          utility::Visitor{
              [this, &timer_ping](messages::ue::ConfigResponse& msg) {
                if (msg._imei != _ctxt.imei()) {
                  std::cout << "INCORRECT IMEI\n";
                  closeConnection();
                  return;
                }
                std::cout << "????\n\n";
                _ctxt.setTTLUE(msg._ttl);
                timer_ping.setWaitTime(
                    static_cast<uint64_t>(static_cast<double>(msg._ttl) / kTtlcoef));
                timer_ping.restart();
                if (!this->_attached && !this->_attachment_in_process) {
                  std::thread attach_procedure(&Exchanger::attachTask, this);
                  attach_procedure.detach();
                }
              },

              [this, &enodeb_list, &max_enodeb_idx](messages::ue::MeasurementResponse& msg) {
                if (msg._imei != _ctxt.imei()) {
                  std::cout << "INCORRECT IMEI\n";
                  closeConnection();
                  return;
                }
                enodeb_list.insert({msg._info._enodeb_id, msg._info._enodeb_power});
                if (!enodeb_list.contains(max_enodeb_idx)) {
                  max_enodeb_idx = msg._info._enodeb_id;
                }

                if (enodeb_list.contains(max_enodeb_idx) &&
                    enodeb_list.at(max_enodeb_idx) < msg._info._enodeb_power) {
                  max_enodeb_idx = msg._info._enodeb_id;
                };
              }},
          *pooling);
      _poolingBufferQ.pop();
      pooling = _poolingBufferQ.front();
    }
    if (!timer_ping.checkTimer()) {
      timer_ping.restart();
      pushToSend(messages::ue::MeasurementRequest{._x = _x, ._imei = _ctxt.imei()});
      pushToSend(messages::ue::MeasurementReport{._enodeb_idx = _picked_enodeb});
    }

    if (_picked_enodeb != max_enodeb_idx) {
      _picked_enodeb = max_enodeb_idx;
    }
  }
}

void Exchanger::attachTask()
{
  _attachment_in_process = true;
  std::cout << "start coonection\n";
  while (_in_active && !_attached) {
    pushToSend(messages::ue::AttachRequest{
        ._imei = _ctxt.imei(),
        ._imsi = _ctxt.imsi(),
        ._msisdn = _ctxt.msisdn(),
    });

    while (_attachment_recv_q.size() == 0 && _in_active) {
      std::this_thread::sleep_for(kSleepTime);
    }
    if (!_in_active) {
      break;
    }
    messages::ue::AuthRequest msg =
        std::get<messages::ue::AuthRequest>(*_attachment_recv_q.front());

    _ctxt = ue::UeContext(msg._tmsi, _ctxt.ttlUe(), _ctxt.imei(), this->_ctxt.msisdn(),
                          this->_ctxt.imsi());

    _attachment_recv_q.pop();
    pushToSend(messages::ue::AuthResponse{._imei = _ctxt.imei(), ._tmsi = _ctxt.tmsi()});

    while (_attachment_recv_q.size() == 0 && _in_active) {
      std::this_thread::sleep_for(kSleepTime);
    }
    if (!_in_active) {
      break;
    }

    auto* msg_activate = _attachment_recv_q.front();

    auto msg_result =
        std::get<messages::ue::AttachResponse>(*msg_activate);  //what if attach failed?

    std::cout << "AUTH DONE CORRECTLY!\n";
    _attachment_recv_q.pop();
    _attached = true;
    _attachment_in_process = false;
  }
}
}  // namespace ue

#include "exchange_ue.hpp"
#include <byteswap.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <array>
#include <bit>
#include <cerrno>
#include <iterator>
#include <unordered_map>
#include <variant>
#include "ip_addr.hpp"
#include "logger.hpp"
#include "resources_test.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"

namespace ue {
const net_conversion_map& getMap()
{
  const static net_conversion_map receive_work{
      {utility::MessageFlag::SMSSend,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::SmsReqNet*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::SmsReqNet>(message);
           msg._msisdn = std::byteswap(msg._msisdn);
           msg._tmsi = std::byteswap(msg._tmsi);
           msg._smsid = std::byteswap(msg._smsid);
         }
       }},
      {utility::MessageFlag::SMSStatus,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::AcknowledgmentResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::AcknowledgmentResponse>(message);
           msg._tmsi = std::byteswap(msg._tmsi);
           msg._message_id = std::byteswap(msg._message_id);
         }
       }},  //function to set status
      {utility::MessageFlag::MeasureRequest,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::MeasurementRequest*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::MeasurementRequest>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._x = std::byteswap(msg._x);
         }
       }},
      {utility::MessageFlag::MeasureResponse,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::MeasurementResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::MeasurementResponse>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._info._enodeb_power = std::byteswap(msg._info._enodeb_power);
           msg._info._enodeb_id = std::byteswap(msg._info._enodeb_id);
         }
       }},
      {utility::MessageFlag::AttachRequest,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::AttachRequest*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::AttachRequest>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._msisdn = std::byteswap(msg._msisdn);
           msg._imsi = std::byteswap(msg._imsi);
         }
       }},
      {utility::MessageFlag::AttachResponse,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::AttachResponse*>(&message);
         auto& msg = std::get<utility::AttachResponse>(message);
       }},
      {utility::MessageFlag::MeasurementReport,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::MeasurementReport*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::MeasurementReport>(message);
           msg._enodeb_idx = std::byteswap(msg._enodeb_idx);
         }
       }},
      {utility::MessageFlag::ConfigurationResp,
       [](utility::UEMessageData& message) {
         message = *std::bit_cast<utility::ConfigResponse*>(&message);
         if constexpr (std::endian::native == std::endian::little) {
           auto& msg = std::get<utility::ConfigResponse>(message);
           msg._imei = std::byteswap(msg._imei);
           msg._ttl = std::byteswap(msg._ttl);
         }
       }},
  };
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
  std::visit(utility::Visitor{
                 [this](std::variant<utility::MeasurementRequest, utility::MeasurementReport>
                            measurement) { _poolingSendQ.push(measurement); },
                 [this](std::string&& str) { _sendSmsQ.push(std::move(str)); },
                 [this](auto attach) { _attachment_send_q.push(attach); },
                 [this](utility::AcknowledgmentRequest ack) { _acknowledgementQ.push(ack); },
             },
             std::move(msg));
}

void Exchanger::pushToRecv(recv_type&& msg)
{
  std::visit(
      utility::Visitor{
          [this](
              std::variant<utility::ConfigResponse, utility::MeasurementResponse> measurement) {
            _poolingBufferQ.push(measurement);
          },
          [this](utility::SmsReqNet str) { _receivedSmsQ.push(str); },
          [this](auto attach) {
            _attachment_recv_q.push(std::forward<decltype(attach)>(attach));
          },
          [this](utility::AcknowledgmentResponse ack) { _receivedSmsStatusQ.push(ack); },
      },
      std::move(msg));
}
void Exchanger::sendTask()
{
  //need to change logger behaviour in multithread mode, need to lock less
  //and run it separate thread
  // logging::MultithreadPresets::functionCall();
  std::array<utility::UEMessage, kSendQueueNumber> packets;

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
    auto* it = std::prev(packets.begin());
    if (utility::AcknowledgmentRequest* data = _acknowledgementQ.front(); is_ack) {
      auto it_func = _receive_map.at(utility::MessageFlag::SMSStatus);
      *std::next(it, 1) =
          utility::UEMessage{._msg_type = utility::MessageFlag::SMSStatus, ._data = *data};
      it_func(it->_data);
      _acknowledgementQ.pop();
    }

    if (std::string* data = _sendSmsQ.front(); is_sms && _attached) {
      auto it_func = _receive_map.at(utility::MessageFlag::SMSStatus);
      *std::next(it, 1) =
          utility::UEMessage{._msg_type = utility::MessageFlag::SMSSend,
                             ._data = utility::SmsReqNet{._tmsi = _ctxt.tmsi(),
                                                         ._msisdn = _ctxt.msisdn(),
                                                         ._smsid = 0,
                                                         ._sms = {*data->data()}}};
      it_func(it->_data);
      _sendSmsQ.pop();
    }

    if (auto* data = _poolingSendQ.front(); is_pool) {
      std::visit(utility::Visitor{
                     [&it, this](utility::MeasurementRequest req) {
                       *std::next(it, 1) = utility::UEMessage{
                           ._msg_type = utility::MessageFlag::MeasureRequest, ._data = req};
                       auto it_func = _receive_map.at(utility::MessageFlag::MeasureRequest);
                       it_func(it->_data);
                     },
                     [&it, this](utility::MeasurementReport req) {
                       *std::next(it, 1) = utility::UEMessage{
                           ._msg_type = utility::MessageFlag::MeasurementReport, ._data = req};
                       auto it_func = _receive_map.at(utility::MessageFlag::MeasurementReport);
                       it_func(it->_data);
                     }},
                 *data);
      _poolingSendQ.pop();
    }

    if (auto* data = _attachment_send_q.front(); is_attach) {
      std::visit(utility::Visitor{
                     [&it, this](utility::AttachRequest req) {
                       *std::next(it, 1) = utility::UEMessage{
                           ._msg_type = utility::MessageFlag::AttachRequest, ._data = req};
                       auto it_func = _receive_map.at(utility::MessageFlag::AttachRequest);
                       it_func(it->_data);
                     },
                     [&it, this](utility::AuthResponse req) {
                       *std::next(it, 1) = utility::UEMessage{
                           ._msg_type = utility::MessageFlag::AuthResponse, ._data = req};
                       auto it_func = _receive_map.at(utility::MessageFlag::AuthResponse);
                       it_func(it->_data);
                     }},
                 *data);
      _attachment_send_q.pop();
    }
    ssize_t status =
        send(_socket, packets.begin(), ((it - it_begin) + 1) * sizeof(utility::UEMessage), 0);

    if (status == -1) {
      closeConnection();
      std::cout << "SEND ERROR\n";  //temp
    }
  }
}

void Exchanger::receiveTask()
{
  std::array<utility::UEMessage, kSendQueueNumber> packets;

  while (_in_active) {
    ssize_t received_amount = recv(_socket, packets.begin(), sizeof(packets), MSG_DONTWAIT);

    if (received_amount == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      std::cout << "RECEIVE ERROR\n";
      closeConnection();
      break;
    }
    const auto* it_end = std::next(
        packets.end(), -static_cast<int64_t>((received_amount / sizeof(utility::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      auto cast_fn = _receive_map.find(it->_msg_type);
      cast_fn != _receive_map.end() ? cast_fn->second(it->_data) : closeConnection();
      if (!_in_active) {
        break;
      }
      std::visit(
          utility::Visitor{
              [this](auto&& msg) { pushToRecv(msg); }, [](utility::AcknowledgmentRequest) {},
              [](utility::MeasurementRequest) {}, [](utility::MeasurementReport) {},
              [](utility::AttachRequest) {}, [](utility::AuthResponse) {},
              [](utility::AttachResponse) {}, [](std::array<char, utility::kMsgDataSize>) {}},
          it->_data);
    }
  }
}

ConnectionStatus Exchanger::createSocket()
{
  _socket = ::socket(AF_INET, SOCK_STREAM, 0);
  if (_socket == -1) {
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't create socket to {}", _ip_addr));
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
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't connect to {}", _ip_addr));
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
  pr_utils::Timer timer_ping(_ctxt.ttlUe());
  pushToSend(utility::MeasurementRequest{._imei = _ctxt.imei(), ._x = this->_x});

  rigtorp::SPSCQueue<size_t> indexes_to_remove{2};
  while (_in_active) {
    while (indexes_to_remove.size() != 0) {
      enodeb_list.erase(*indexes_to_remove.front());
      indexes_to_remove.pop();
    }
    max_enodeb_idx = findmaxPower(enodeb_list);

    std::variant<utility::ConfigResponse, utility::MeasurementResponse>* msg =
        _poolingBufferQ.front();

    if (msg != nullptr) {
      std::visit(
          utility::Visitor{
              [this, &enodeb_list, &max_power,
               &max_enodeb_idx](utility::MeasurementResponse& control) {
                if (control._imei != _ctxt.imei()) {
                  std::cout << "WRONG IMEI\n";
                  this->closeConnection();
                  return;
                }
                enodeb_list[control._info._enodeb_id] = control._info._enodeb_power;
                if (control._info._enodeb_power > max_power) {
                  max_power = control._info._enodeb_power;
                  max_enodeb_idx = control._info._enodeb_id;
                }
              },
              [this, &timer_ping, &enodeb_list, &max_enodeb_idx](utility::ConfigResponse& cfg) {
                if (cfg._status == utility::EnodeBStatus::DISCONNECT) {
                  enodeb_list.erase(max_enodeb_idx);
                  max_enodeb_idx = findmaxPower(enodeb_list);
                  pushToSend(utility::MeasurementReport{._enodeb_idx = max_enodeb_idx});
                  return;
                }

                if (cfg._imei != this->_ctxt.imei()) {
                  std::cout << "WRONG IMEI\n";
                  this->closeConnection();
                  return;
                }
                this->_ctxt = ue::UeContext(this->_ctxt.tmsi(), cfg._ttl, cfg._imei,
                                            this->_ctxt.msisdn(), this->_ctxt.imsi());
                timer_ping.setWaitTime(cfg._ttl);
                timer_ping.restart();
              }},
          *msg);
      continue;
      _poolingBufferQ.pop();
    }

    if (_picked_enodeb != max_enodeb_idx) {
      _picked_enodeb = max_enodeb_idx;
      pushToSend(utility::MeasurementReport{._enodeb_idx = max_enodeb_idx});
    }

    if (!_attached) {
      if (0 != enodeb_list.size() && !_attachment_in_process) {
        std::thread attach_procedure(&Exchanger::attachTask, this, std::ref(indexes_to_remove));
        attach_procedure.detach();
      }
      continue;
    }

    if (_attached && !timer_ping.checkTimer()) {
      continue;
    }
    timer_ping.restart();
    pushToSend(utility::MeasurementRequest{._imei = _ctxt.imei(), ._x = this->_x});
  }
}

void Exchanger::attachTask(rigtorp::SPSCQueue<size_t>& indexes_to_remove)
{
  _attachment_in_process = true;
  auto status = createSocket();
  if (status != ConnectionStatus::valid_connection) {
    closeConnection();
    return;
  }

  while (_in_active && !_attached) {
    pushToSend(utility::AttachRequest{
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
    utility::AuthRequest msg = std::get<utility::AuthRequest>(*_attachment_recv_q.front());

    _ctxt = ue::UeContext(msg._tmsi, _ctxt.ttlUe(), _ctxt.imei(), this->_ctxt.msisdn(),
                          this->_ctxt.imsi());

    _attachment_recv_q.pop();
    pushToSend(utility::AuthResponse{._imei = _ctxt.imei(), ._tmsi = _ctxt.tmsi()});

    while (_attachment_recv_q.size() == 0 && _in_active) {
      std::this_thread::sleep_for(kSleepTime);
    }
    if (!_in_active) {
      break;
    }

    auto* msg_activate = _attachment_recv_q.front();

    auto msg_result =
        std::get<utility::AttachResponse>(*msg_activate);  //what if attach failed?

    std::cout << "AUTH DONE CORRECTLY!\n";
    _attachment_recv_q.pop();
    _attached = true;
    _attachment_in_process = false;
  }
}
}  // namespace ue

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
    if (utility::AcknowledgmentReq* data = _acknowledgementQ.front(); is_ack) {
      data->_tmsi_d = htonl(data->_tmsi_d);
      *std::next(it, 1) =
          utility::UEMessage{._msg_type = utility::MessageFlag::SMSStatus, ._data = *data};
      _acknowledgementQ.pop();
    }

    if (std::string* data = _sendSmsQ.front(); is_sms && _attached) {
      if constexpr (std::endian::native == std::endian::little) {
        *std::next(it, 1) = utility::UEMessage{
            ._msg_type = utility::MessageFlag::SMSSend,
            ._data = utility::SmsReqNet{._tmsi = std::byteswap(_ctxt.tmsi()),
                                        ._msisdn = std::byteswap(_ctxt.msisdn()),
                                        ._smsid = std::byteswap(0),
                                        ._sms = {*data->data()}}};
      }
      else {
        *std::next(it, 1) =
            utility::UEMessage{._msg_type = utility::MessageFlag::SMSSend,
                               ._data = utility::SmsReqNet{._tmsi = _ctxt.tmsi(),
                                                           ._msisdn = _ctxt.msisdn(),
                                                           ._smsid = 0,
                                                           ._sms = {*data->data()}}};
      }
      _sendSmsQ.pop();
    }

    if (auto* data = _poolingSendQ.front(); is_pool) {
      std::visit(
          utility::Visitor{[&it](utility::MeasurementReq req) {
                             if constexpr (std::endian::native == std::endian::little) {
                               req._imei = std::byteswap(req._imei);
                               req._x = std::byteswap(req._x);
                             }

                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::RangeReq, ._data = req};
                           },
                           [&it](utility::MeasurementConnectReq req) {
                             if constexpr (std::endian::native == std::endian::little) {
                               req._enodeb_idx = std::byteswap(req._enodeb_idx);
                             }
                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::Reconnect, ._data = req};
                           }},
          *data);
      _poolingSendQ.pop();
    }

    if (auto* data = _attachment_send_q.front(); is_attach) {
      std::visit(
          utility::Visitor{[&it](utility::AttachReq req) {
                             if constexpr (std::endian::native == std::endian::little) {
                               req._imei = std::byteswap(req._imei);
                               req._enodeb_number = std::byteswap(req._enodeb_number);
                               req._imsi = std::byteswap(req._imsi);
                               req._msisdn = std::byteswap(req._msisdn);
                             }

                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::AttachReq, ._data = req};
                           },
                           [&it](utility::AuthReq req) {
                             if constexpr (std::endian::native == std::endian::little) {
                               req._imei = std::byteswap(req._imei);
                               req._tmsi = std::byteswap(req._tmsi);
                             }

                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::AuthReq, ._data = req};
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
      std::visit(utility::Visitor{
                     [this](auto&& msg) { pushToRecv(msg); }, [](utility::AcknowledgmentReq) {},
                     [](utility::MeasurementReq) {}, [](utility::MeasurementConnectReq) {},
                     [](utility::AttachReq) {}, [](utility::AuthReq) {},
                     [](utility::AuthResp) {}, [](std::array<char, utility::kMsgDataSize>) {}},
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
  pushToSend(utility::MeasurementReq{._imei = _ctxt.imei(), ._x = this->_x});

  rigtorp::SPSCQueue<size_t> indexes_to_remove{2};
  while (_in_active) {
    while (indexes_to_remove.size() != 0) {
      enodeb_list.erase(*indexes_to_remove.front());
      indexes_to_remove.pop();
    }
    max_enodeb_idx = findmaxPower(enodeb_list);

    std::variant<utility::ConfigResp, utility::MeasurementResp>* msg = _poolingBufferQ.front();

    if (msg != nullptr) {
      std::visit(
          utility::Visitor{
              [this, &enodeb_list, &max_power,
               &max_enodeb_idx](utility::MeasurementResp& control) {
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
              [this, &timer_ping, &enodeb_list, &max_enodeb_idx](utility::ConfigResp& cfg) {
                if (cfg._status == utility::EnodeBStatus::DISCONNECT) {
                  enodeb_list.erase(max_enodeb_idx);
                  max_enodeb_idx = findmaxPower(enodeb_list);
                  pushToSend(utility::MeasurementConnectReq{._enodeb_idx = max_enodeb_idx});
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
      pushToSend(utility::MeasurementConnectReq{._enodeb_idx = max_enodeb_idx});
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
    pushToSend(utility::MeasurementReq{._imei = _ctxt.imei(), ._x = this->_x});
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
    pushToSend(utility::AttachReq{._imei = _ctxt.imei(),
                                  ._imsi = _ctxt.imsi(),
                                  ._msisdn = _ctxt.msisdn(),
                                  ._enodeb_number = _picked_enodeb});

    while (_attachment_recv_q.size() == 0 && _in_active) {
      std::this_thread::sleep_for(kSleepTime);
    }
    if (!_in_active) {
      break;
    }
    utility::AttachResponse msg =
        std::get<utility::AttachResponse>(*_attachment_recv_q.front());

    _ctxt = ue::UeContext(msg._tmsi, _ctxt.ttlUe(), _ctxt.imei(), this->_ctxt.msisdn(),
                          this->_ctxt.imsi());

    _attachment_recv_q.pop();
    pushToSend(utility::AuthReq{._imei = _ctxt.imei(), ._tmsi = _ctxt.tmsi()});

    while (_attachment_recv_q.size() == 0 && _in_active) {
      std::this_thread::sleep_for(kSleepTime);
    }
    if (!_in_active) {
      break;
    }

    auto* msg_activate = _attachment_recv_q.front();

    auto msg_result = std::get<utility::AuthResp>(*msg_activate);  //what if attach failed?
    if (msg_result._status == utility::EnodeBStatus::DISCONNECT) {

      indexes_to_remove.push(_picked_enodeb);
      return;
    }
    std::cout << "AUTH DONE CORRECTLY!\n";
    _attachment_recv_q.pop();
    _attached = true;
    _attachment_in_process = false;
  }
}
}  // namespace ue

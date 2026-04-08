#include "exchange_ue.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <array>
#include <iterator>
#include <variant>
#include "ip_addr.hpp"
#include "logger.hpp"
#include "resources_test.hpp"
#include "utility.hpp"

namespace ue {
void Exchanger::sendTask()
{
  //need to change logger behaviour in multithread mode, need to lock less
  //and run it separate thread
  // logging::MultithreadPresets::functionCall();
  std::array<utility::UEMessage, kSendQueueNumber> packets;
  const auto* it_end = packets.end();
  while (_in_active) {
    _send_queue_sm.acquire();
    _send_queue_sm.release();

    auto* it = std::prev(packets.begin());
    if (utility::AcknowledgmentUE* data = _acknowledgementQ.front(); nullptr != data) {
      _send_queue_sm.acquire();
      *std::next(it, 1) =
          utility::UEMessage{._msg_type = utility::MessageFlag::SMSStatus, ._data = *data};
      _acknowledgementQ.pop();
    }

    if (std::string* data = _sendSmsQ.front(); nullptr != data) {
      _send_queue_sm.acquire();
      *std::next(it, 1) =
          utility::UEMessage{._msg_type = utility::MessageFlag::SMSSend,
                             ._data = utility::SmsReqNet{._tmsi = _ctxt.tmsi(),
                                                         ._msisdn = _ctxt.msisdn(),
                                                         ._smsid = 0,
                                                         ._sms = {*data->data()}}};
      _sendSmsQ.pop();
    }

    if (auto* data = _poolingSendQ.front(); nullptr != data) {
      _send_queue_sm.acquire();
      std::visit(
          utility::Visitor{[&it](utility::MeasurementReq req) {
                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::RangeReq, ._data = req};
                           },
                           [&it](utility::MeasurementReport req) {
                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::Reconnect, ._data = req};
                           }},
          *data);
      _acknowledgementQ.pop();
    }

    if (auto* data = _attachment_send_q.front(); nullptr != data) {
      _send_queue_sm.acquire();
      std::visit(
          utility::Visitor{[&it](utility::AttachReq req) {
                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::AttachReq, ._data = req};
                           },
                           [&it](utility::AuthReq req) {
                             *std::next(it, 1) = utility::UEMessage{
                                 ._msg_type = utility::MessageFlag::AuthReq, ._data = req};
                           }},
          *data);
      _acknowledgementQ.pop();
    }

    ssize_t status =
        send(_socket, packets.begin(), (it_end - it) * sizeof(utility::UEMessage), 0);

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
    ssize_t received_amount = recv(_socket, packets.begin(), sizeof(packets), 0);

    if (received_amount == -1) {
      std::cout << "RECEIVE ERROR\n";
      closeConnection();
      break;
    }
    const auto* it_end = std::next(
        packets.end(), -static_cast<int64_t>((received_amount / sizeof(utility::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      const auto function = _receive_map.find(it->_msg_type);
      function != _receive_map.end() ? function->second(std::move(it->_data))
                                     : closeConnection();
    }
  }
}

ConnectionStatus Exchanger::createSocket()
{
  _socket = socket(AF_INET, SOCK_STREAM, 0);
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
void Exchanger::pingTask()
{
  //map of all received enodeb, pick whenever, pick only if signal power greater than threshold
  while (_in_active) {
    //if (false) {}
    std::variant<utility::ConfigReq, utility::MeasurementControl>* msg =
        _poolingBufferQ.front();
  }
}
void Exchanger::attachTask()
{
  auto status = createSocket();
  if (status != ConnectionStatus::valid_connection) {
    closeConnection();
    return;
  }

  while (_in_active && _attach_sm.try_acquire()) {
    _attach_sm.acquire();

    _attachment_send_q.push(utility::AttachReq{._imei = _ctxt.imei(),
                                               ._imsi = _ctxt.imsi(),
                                               ._msisdn = _ctxt.msisdn(),
                                               ._enodeb_number = _picked_enodeb});

    _attach_sm.acquire();
    utility::AttachResponse msg =
        std::get<utility::AttachResponse>(*_attachment_recv_q.front());

    if (_ctxt.tmsi() != msg._tmsi) {
      closeConnection();
      std::cout << "AUTH FAILED!\n";
      break;
    }
    _attachment_recv_q.pop();
    _attachment_send_q.push(utility::AuthReq{._imei = _ctxt.imei(), ._tmsi = _ctxt.tmsi()});

    _attach_sm.acquire();
    std::cout << "AUTH DONE CORRECTLY!\n";
    _attachment_recv_q.pop();
    _attached = true;
  }
}
}  // namespace ue

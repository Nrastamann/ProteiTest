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
      *std::next(it, 1) = utility::UEMessage{
          ._msg_type = utility::MessageFlag::SMSSend,
          ._data = utility::SmsReqNet{
              ._tmsi = _ctxt.tmsi(), ._msisdn = _ctxt.msisdn(), ._sms = {*data->data()}}};
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
      _in_active = false;
      close(_socket);
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
      _in_active = false;
      close(_socket);
      break;
    }
    const auto* it_end = std::next(
        packets.end(), -static_cast<int64_t>((received_amount / sizeof(utility::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      const auto function = _receive_map.find(it->_msg_type);
      function != _receive_map.end() ? function->second(std::move(it->_data)) : [this]() {
        this->_in_active = false;
        this->_attached = false;
        this->_socket = close(_socket);
      }();
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
    _in_active = false;
    close(_socket);
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't connect to {}", _ip_addr));
    return ConnectionStatus::connection_err;
  }
  return ConnectionStatus::valid_connection;
}
ConnectionStatus Exchanger::attach()
{
  auto status = createSocket();
  if (status != ConnectionStatus::valid_connection) {
    return status;
  }

  return status;
}
}  // namespace ue

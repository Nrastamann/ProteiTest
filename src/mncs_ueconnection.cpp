#include "mncs_ueconnection.hpp"
#include <sys/socket.h>
#include "utility.hpp"
namespace mncs {
void UEConnection::sendDirectly(utility::UEMessage&& msg)
{
  if (_socket != -1) {
    auto cast_fn = _map.find(msg._msg_type);
    cast_fn != _map.end() ? cast_fn->second(msg._data) : terminate();
    if (-1 == _socket) {
      return;
    }

    ssize_t res = send(_socket, &msg._data, utility::kMsgSize, 0);
    if (-1 == res) {
      terminate();
    }
  }
}

void UEConnection::run()
{
  std::array<utility::UEMessage, recv_buffer_len> packets;
  utility::UEMessage buf;
  while (_socket != -1) {
    while (this->_connected_station->_sendQ.size() != 0) {
      std::visit(utility::Visitor{[&buf](utility::SmsReqNet&& msg) {
                                    buf._msg_type = utility::MessageFlag::SMSSend;
                                    buf._data = std::move(msg);
                                  },
                                  [&buf](utility::AcknowledgmentResp&& msg) {
                                    buf._msg_type = utility::MessageFlag::SMSStatus;
                                    buf._data = std::move(msg);
                                  },
                                  [&buf](utility::MeasurementResp&& msg) {
                                    buf._msg_type = utility::MessageFlag::RangeResp;
                                    buf._data = std::move(msg);
                                  },
                                  [&buf](utility::AttachResponse&& msg) {
                                    buf._msg_type = utility::MessageFlag::AttachResp;
                                    buf._data = std::move(msg);
                                  },

                                  [&buf](utility::AuthResp&& msg) {
                                    buf._msg_type = utility::MessageFlag::AuthResp;
                                    buf._data = std::move(msg);
                                  },
                                  [&buf](utility::ConfigResp&& msg) {
                                    buf._msg_type = utility::MessageFlag::Configuration;
                                    buf._data = std::move(msg);
                                  },
                                  [](auto&&) {}},
                 std::move(*this->_connected_station->_sendQ.front()));
      this->_connected_station->_sendQ.pop();
      ssize_t res = ::send(_socket, &buf, utility::kMsgSize, 0);
      if (res == -1) {
        this->terminate();
        return;
      }
    }
    ssize_t res = recv(_socket, packets.begin(), utility::kMsgSize, MSG_DONTWAIT);
    if (res == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      std::cout << "RECEIVE ERROR\n";
      terminate();
      break;
    }

    const auto* it_end =
        std::next(packets.end(), -static_cast<int64_t>((res / sizeof(utility::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      auto cast_fn = _map.find(it->_msg_type);
      cast_fn != _map.end() ? cast_fn->second(it->_data) : terminate();
      if (-1 != _socket) {
        break;
      }
      std::visit(utility::Visitor{
                     [this](utility::SmsReqNet&& msg) {
                       this->_connected_station->_receiveQ.push(std::move(msg));
                     },
                     [](auto&&) {},
                     [this](utility::AcknowledgmentReq&& msg) {
                       this->_connected_station.;  //to mme
                     },
                     [this](utility::MeasurementReq&& msg) {
                       for (auto& enodeb : *_enodeb_list) {
                         enodeb.second._receiveQ.push(std::move(msg));
                       }
                     },
                     [this](utility::MeasurementConnectReq&& msg) {
                       _enodeb_list->at(msg._enodeb_idx)._receiveQ.push(std::move(msg));
                     },
                     [this](utility::AttachReq&& msg) {
                       this->_connected_station->_receiveQ.push(std::move(msg));
                     },
                     [this](utility::AuthReq&& msg) {},  //to mme},
                 },
                 std::move(it->_data));
    }
  }
}
}  // namespace mncs

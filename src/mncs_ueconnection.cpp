#include "mncs_ueconnection.hpp"
#include <sys/socket.h>
#include <bit>
#include <chrono>
#include <thread>
#include <utility>
#include <variant>
#include "utility.hpp"
static constexpr size_t kSleepTime{5};
namespace mncs {

void UEConnection::run()
{
  std::array<utility::UEMessage, kBufferLength> packets;
  auto* it_begin = packets.begin();
  utility::UEMessageData msg;
  while (_socket != -1) {
    auto* it = std::prev(packets.begin());

    auto* message_to_send = _messages_send.front();
    auto* measurement_results = _measurementQ.front();

    while (message_to_send != nullptr || measurement_results != nullptr ||
           packets.end() != it_begin) {
      while (measurement_results != nullptr) {
        msg = {*measurement_results};
        _socket_data_processing.at(utility::MessageFlag::MeasureResponse)(msg);
        *std::next(it, 1) = utility::UEMessage{
            ._msg_type = utility::MessageFlag::MeasureResponse, ._data = msg};
        _measurementQ.pop();
        measurement_results = _measurementQ.front();
      }
      while (message_to_send != nullptr) {
        utility::MessageFlag flag = std::visit(
            utility::Visitor{
                [](auto&) { return utility::MessageFlag::SMSStatus; },
                [](utility::SmsReqNet&) { return utility::MessageFlag::SMSSend; },
                [](utility::ConfigResponse&) {
                  return utility::MessageFlag::MeasurementReport;
                },
                [](utility::MeasurementResponse&) {
                  return utility::MessageFlag::MeasureResponse;
                },
                [](utility::AcknowledgmentResponse&) {
                  return utility::MessageFlag::AttachResponse;
                },
                [](utility::AttachResponse&) { return utility::MessageFlag::AttachResponse; },
                [](utility::AuthRequest&) { return utility::MessageFlag::AuthRequest; }},
            *message_to_send);
        _socket_data_processing.at(flag)(*message_to_send);
        *std::next(it, 1) = utility::UEMessage{._msg_type = flag, ._data = *message_to_send};

        _messages_send.pop();
        message_to_send = _messages_send.front();
      }
    }
    ssize_t status =
        send(_socket, packets.begin(), ((it - it_begin) + 1) * sizeof(utility::UEMessage), 0);

    if (status == -1) {
      terminate();
      std::cout << "SEND ERROR\n";  //temp
    }
    ssize_t res = recv(_socket, packets.begin(), sizeof(packets), MSG_DONTWAIT);
    if (res == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime));
        continue;
      }
      std::cout << "RECEIVE ERROR\n";
      terminate();
      break;
    }

    const auto* it_end =
        std::next(packets.end(), -static_cast<int64_t>((res / sizeof(utility::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      auto cast_fn = _socket_data_processing.find(it->_msg_type);
      cast_fn != _socket_data_processing.end() ? cast_fn->second(it->_data) : terminate();
      if (-1 != _socket) {
        break;
      }

      std::visit(utility::Visitor{[this](auto& msg) { _messages_recv.push(msg); },
                                  [this](utility::MeasurementRequest& msg) {
                                    for (auto& station : _enodeb_list) {
                                      uint64_t power = station.second->getPower(msg._x);
                                      _measurementQ.push(utility::MeasurementResponse{
                                          ._imei = msg._imei,
                                          ._info = {._enodeb_power = power,
                                                    ._enodeb_id = station.second->getIdx()}});
                                    }
                                  }},
                 it->_data);
    }
  }
}
}  // namespace mncs

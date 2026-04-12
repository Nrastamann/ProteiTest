#include "mncs_ueconnection.hpp"
#include <sys/socket.h>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include "mncs_messages.hpp"
#include "ue_messages.hpp"
#include "utility.hpp"

static constexpr size_t kSleepTime{5};
namespace mncs {
void UEConnection::sendTask()
{
  std::array<messages::ue::UEMessage, kBufferLength> packets;
  auto* it_begin = packets.begin();
  messages::ue::UEMessageData msg;
  while (_socket != -1) {
    auto* it = std::prev(packets.begin());

    auto* message_to_send = _messages_send.front();
    auto* measurement_results = _measurementQ.front();

    while (message_to_send != nullptr || measurement_results != nullptr ||
           packets.end() != it_begin) {
      while (measurement_results != nullptr) {
        msg = {*measurement_results};
        _socket_data_processing.at(messages::ue::MessageFlag::MeasureResponse)(msg);
        *std::next(it, 1) = messages::ue::UEMessage{
            ._msg_type = messages::ue::MessageFlag::MeasureResponse, ._data = msg};
        _measurementQ.pop();
        measurement_results = _measurementQ.front();
      }
      while (message_to_send != nullptr) {
        messages::ue::MessageFlag flag = std::visit(
            utility::Visitor{
                [](auto&) { return messages::ue::MessageFlag::SMSStatus; },
                [](messages::ue::AuthResponse&) {
                  return messages::ue::MessageFlag::AuthResponse;
                },  //error during attach
                [](messages::ue::SmsReqNet&) { return messages::ue::MessageFlag::SMSSend; },
                [](messages::ue::ConfigResponse&) {
                  return messages::ue::MessageFlag::MeasurementReport;
                },
                [](messages::ue::AcknowledgmentResponse&) {
                  return messages::ue::MessageFlag::AttachResponse;
                },
                [](messages::ue::AttachResponse&) {
                  return messages::ue::MessageFlag::AttachResponse;
                },
                [](messages::ue::AuthRequest&) {
                  return messages::ue::MessageFlag::AuthRequest;
                }},
            *message_to_send);
        _socket_data_processing.at(flag)(*message_to_send);
        *std::next(it, 1) =
            messages::ue::UEMessage{._msg_type = flag, ._data = *message_to_send};

        _messages_send.pop();
        message_to_send = _messages_send.front();
      }
    }
    ssize_t status = send(_socket, packets.begin(),
                          ((it - it_begin) + 1) * sizeof(messages::ue::UEMessage), 0);

    if (status == -1) {
      terminate();
      std::cout << "SEND ERROR\n";  //temp
    }
  }
}

void UEConnection::receiveTask(
    std::unordered_map<size_t, std::unique_ptr<BaseStation>>& enodeb_list)
{
  std::array<messages::ue::UEMessage, kBufferLength> packets;
  messages::ue::UEMessageData msg;
  while (-1 != _socket) {
    ssize_t res = recv(_socket, packets.begin(), sizeof(packets), MSG_DONTWAIT);
    if (res == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime / 2));
        continue;
      }
      std::cout << "RECEIVE ERROR\n";
      terminate();
      break;
    }

    const auto* it_end = std::next(
        packets.end(), -static_cast<int64_t>((res / sizeof(messages::ue::UEMessage))));

    for (auto* it = packets.begin(); it != it_end; std::advance(it, 1)) {
      auto cast_fn = _socket_data_processing.find(it->_msg_type);
      cast_fn != _socket_data_processing.end() ? cast_fn->second(it->_data) : terminate();
      if (-1 != _socket) {
        break;
      }

      mncs::UEConnection* ptr = this;
      std::visit(
          utility::Visitor{
              [](auto&) {},
              [this, &enodeb_list, &ptr](messages::ue::MeasurementReport& msg) {
                std::visit(
                    utility::Visitor{
                        [&enodeb_list, msg, &ptr](bool) {
                          enodeb_list.at(msg._enodeb_idx).get()->pushToConnections(ptr);
                        },
                        [&msg, this](size_t id) {
                          id == msg._enodeb_idx
                              ? _messages_recv.push(Ping{})
                              : _messages_recv.push(HandoverStart{._dst_id = msg._enodeb_idx});
                        }},
                    _connected_station);
              },
              [this](messages::ue::AcknowledgmentRequest& msg) { _messages_recv.push(msg); },
              [this](messages::ue::SmsReqNet& msg) { _messages_recv.push(msg); },
              [this, &enodeb_list](messages::ue::AttachRequest& msg) {
                _imei = msg._imei;
                _imsi = msg._imsi;
                _tmsi = 0;

                _messages_recv.push(
                    AttachRequest{._data = msg,
                                  ._id = {._connection_id = _idx,
                                          ._enodeb_id = std::get<size_t>(_connected_station)}});
              },
              [this](messages::ue::AuthResponse& msg) {
                if (msg._tmsi != _tmsi || msg._imei != _imei) {
                  _messages_recv.push(
                      messages::ue::AuthResponse{._imei = msg._imei, ._tmsi = msg._tmsi});
                  return;
                }
                _messages_recv.push(AuthResponse{._tmsi = msg._tmsi});
              },
              [this, &enodeb_list](messages::ue::MeasurementRequest& msg) {
                for (auto& station : enodeb_list) {
                  uint64_t power = station.second->getPower(msg._x);
                  _measurementQ.push(messages::ue::MeasurementResponse{
                      ._imei = msg._imei,
                      ._info = {._enodeb_power = power,
                                ._enodeb_id = station.second->getIdx()}});
                }
                _messages_recv.push(Ping{});
              }},
          it->_data);
    }
  }  // namespace mncs
}
}  // namespace mncs

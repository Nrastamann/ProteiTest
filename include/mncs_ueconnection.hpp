#pragma once
#include <bits/chrono.h>
#include <endian.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstddef>
#include <expected>
#include <thread>
#include <unordered_map>
#include "exchange_ue.hpp"
#include "mncs_messages.hpp"
#include "ue_messages.hpp"

#include "mncs_basestation.hpp"
#include "rigtorp/SPSCQueue.h"

#include "utility.hpp"

namespace mncs {
class BaseStation;
class UEConnection {
  static constexpr size_t kBufferLength{10};

 public:
  UEConnection(std::unordered_map<size_t, std::unique_ptr<BaseStation>>& enodeb_list,
               size_t idx, int socket)
      : _socket(socket), _idx(idx), _connected_station(false)
  {
    std::thread sender(&UEConnection::sendTask, this);
    std::thread receiver(&UEConnection::receiveTask, this, std::ref(enodeb_list));

    sender.detach();
    receiver.detach();
  }

  void sendTask();
  void receiveTask(std::unordered_map<size_t, std::unique_ptr<BaseStation>>& enodeb_list);
  void terminate()
  {
    close(_socket);
    _socket = -1;
  }

  [[nodiscard]] bool isEnded() const { return _socket == -1; }
  void setEnodeb(size_t enodebidx) { _connected_station = enodebidx; }
  void setImei(uint64_t imei) { _imei = imei; }
  void setImsi(uint64_t imsi) { _imsi = imsi; }
  void setTmsi(uint32_t tmsi) { _tmsi = tmsi; }

  [[nodiscard]] uint64_t getImei() const { return _imei; }
  [[nodiscard]] uint64_t getImsi() const { return _imsi; }
  [[nodiscard]] uint32_t getTmsi() const { return _tmsi; }
  [[nodiscard]] std::expected<size_t, bool> getStationId() const
  {
    return std::get<size_t>(_connected_station);  //temp
  }
  [[nodiscard]] size_t getIdx() const { return _idx; }
  void pushToUE(const ::messages::ue::UEMessageData& msg) { _messages_send.push(msg); }

  EnodeBRecv* getFromUE() { return _messages_recv.front(); }
  void popFromUe() { _messages_recv.pop(); }
  utility::default_buffer::iterator getBuffer() { return _buffer_it; }
  void setBuffer(utility::default_buffer::iterator it) { _buffer_it = it; }

 private:
  const ue::net_conversion_map& _socket_data_processing = ue::getMap();
  //message type to enodeb
  rigtorp::SPSCQueue<::messages::ue::UEMessageData> _messages_send{kBufferLength};
  //message from enodeb
  rigtorp::SPSCQueue<EnodeBRecv> _messages_recv{kBufferLength};
  rigtorp::SPSCQueue<::messages::ue::MeasurementResponse> _measurementQ{kBufferLength};

  std::variant<size_t, bool>
      _connected_station;  //atomic counters in shared vs accessing unordered_map
  size_t _idx{};
  utility::default_buffer::iterator _buffer_it{};
  uint64_t _imei{};
  uint64_t _imsi{};
  uint32_t _tmsi{};

  int _socket;
};
}  // namespace mncs

#pragma once
#include <bits/chrono.h>
#include <endian.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include <unordered_map>
#include "exchange_ue.hpp"
#include "mncs_basestation.hpp"
#include "rigtorp/SPSCQueue.h"
#include "utility.hpp"
namespace mncs {
class BaseStation;
class UEConnection {
  static constexpr size_t kBufferLength{10};

 public:
  UEConnection(int socket,
               std::unordered_map<size_t, std::unique_ptr<BaseStation>>& enodeb_list)
      : _socket(socket), _enodeb_list(enodeb_list)
  {
    std::thread worker(&UEConnection::run, this);
    worker.detach();
  }

  void run();
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
  [[nodiscard]] UEConnection* getConnection() { return this; }

 private:
  const ue::net_conversion_map& _socket_data_processing = ue::getMap();
  //message type to enodeb
  rigtorp::SPSCQueue<utility::UEMessageData> _messages_send{kBufferLength};
  //message from enodeb
  rigtorp::SPSCQueue<utility::UEMessageData> _messages_recv{kBufferLength};
  rigtorp::SPSCQueue<utility::MeasurementResponse> _measurementQ{kBufferLength};

  std::unordered_map<size_t, std::unique_ptr<BaseStation>>& _enodeb_list;
  size_t _connected_station{};  //atomic counters in shared vs accessing unordered_map
  uint64_t _imei{};
  uint64_t _imsi{};
  uint32_t _tmsi{};

  int _socket;
};
}  // namespace mncs

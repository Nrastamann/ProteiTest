#pragma once
#include <bits/chrono.h>
#include <endian.h>
#include <sys/socket.h>
#include <unistd.h>
#include <functional>
#include <unordered_map>
#include "exchange_ue.hpp"
#include "mncs_basestation.hpp"
#include "rigtorp/SPSCQueue.h"
#include "utility.hpp"
namespace mncs {
class BaseStation;
static constexpr size_t recv_buffer_len{4};
class UEConnection {
 public:
  UEConnection(int socket) : _socket(socket) {}
  void setList(std::unordered_map<size_t, BaseStation>* ptr) { _enodeb_list = ptr; }
  void setStation(BaseStation* ptr) { _connected_station = ptr; }

  void sendDirectly(utility::UEMessage&& msg);
  void run();
  void terminate()
  {
    close(_socket);
    _socket = -1;
  }

  void pushToBase() {}
  void pushToUe() {}
  void setidx(size_t idx) { _idx = idx; }
  [[nodiscard]] size_t getidx() const { return _idx; }

 private:
  using recv_map =
      std::unordered_map<utility::MessageFlag, std::function<void(utility::UEMessageData&)>>;
  const recv_map& _map = ue::getMap();
  std::unordered_map<size_t, BaseStation>* _enodeb_list{nullptr};
  BaseStation* _connected_station{nullptr};
  size_t _idx{};
  uint64_t _imei{};
  uint64_t _imsi{};
  uint64_t _msisdn{};
  uint32_t _tmsi{};

  int _socket;
};
}  // namespace mncs

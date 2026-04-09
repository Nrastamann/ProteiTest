#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "exchange_ue.hpp"
#include "logger.hpp"
#include "mncs_basestation.hpp"
#include "mncs_ueconnection.hpp"
#include "utility.hpp"
namespace mncs {
class Listener {
  static constexpr size_t kListenNumber{utility::kThreadNum * 2};

 public:
  Listener(uint16_t port, std::unordered_map<size_t, BaseStation>* ptr_to_nodes)
      : _base_station_list(ptr_to_nodes), _port(port)
  {
    _connections.reserve(utility::kThreadNum);
  }

  ~Listener() { close(_socket); }
  Listener(Listener&&) = delete;
  Listener& operator=(Listener&&) = delete;
  Listener& operator=(Listener&) = delete;
  Listener(Listener&) = delete;

  bool startListener();
  void server();
  void closeServer() { _shouldClose = false; }
  [[nodiscard]] bool getStatus() const { return _shouldClose; }

 private:
  using recv_map =
      std::unordered_map<utility::MessageFlag, std::function<void(utility::UEMessageData&)>>;

  std::vector<mncs::UEConnection> _connections;
  std::unordered_map<size_t, mncs::BaseStation>* _base_station_list;
  const recv_map& _map = ue::getMap();
  int _socket{};
  uint16_t _port;
  bool _shouldClose{false};
};
}  // namespace mncs

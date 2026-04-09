#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <vector>
#include "logger.hpp"
#include "mncs_basestation.hpp"
#include "mncs_ueconnection.hpp"
#include "thread_pool.hpp"
#include "utility.hpp"

namespace mncs {
class BaseStation;
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
  std::vector<UEConnection> _connections;
  std::unordered_map<size_t, BaseStation>* _base_station_list;
  int _socket{};
  uint16_t _port;
  bool _shouldClose{false};
};
}  // namespace mncs

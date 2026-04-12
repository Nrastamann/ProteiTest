#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "exchange_ue.hpp"
#include "logger.hpp"
#include "mncs_basestation.hpp"
#include "mncs_ueconnection.hpp"
#include "ue_messages.hpp"
#include "utility.hpp"

namespace mncs {
class Listener {
  static constexpr size_t kListenNumber{utility::kThreadNum * 2};
  static size_t connection_id;

 public:
  Listener(uint16_t port,
           std::unordered_map<size_t, std::unique_ptr<BaseStation>>& ptr_to_nodes)
      : _base_station_list(ptr_to_nodes), _port(port)
  {
    connection_id = 0;
    _connections.reserve(kListenNumber);
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
  using recv_map = std::unordered_map<messages::ue::MessageFlag,
                                      std::function<void(messages::ue::UEMessageData&)>>;

  std::vector<std::unique_ptr<mncs::UEConnection>> _connections;
  std::unordered_map<size_t, std::unique_ptr<mncs::BaseStation>>& _base_station_list;
  const recv_map& _map = ue::getMap();
  int _socket{};
  uint16_t _port;
  bool _shouldClose{false};
};
}  // namespace mncs

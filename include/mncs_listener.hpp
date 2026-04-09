#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "logger.hpp"
#include "mncs_ueconnection.hpp"
#include "thread_pool.hpp"

namespace mncs {
static constexpr size_t kThreadNum{8};

class Listener {
  static constexpr size_t kListenNumber{kThreadNum * 2};

 public:
  explicit Listener(uint16_t port) : _port(port) { _connections.reserve(kThreadNum); }

  ~Listener() { close(_socket); }
  Listener(Listener&&) = delete;
  Listener& operator=(Listener&&) = delete;
  Listener& operator=(Listener&) = delete;
  Listener(Listener&) = delete;

  bool startListener();
  void server();
  void closeServer() { _shouldClose = false; }

 private:
  std::vector<UEConnection> _connections;
  int _socket{};
  uint16_t _port;
  bool _shouldClose{};
};
}  // namespace mncs

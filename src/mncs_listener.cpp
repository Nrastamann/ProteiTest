#include "mncs_listener.hpp"
#include <cstddef>
#include <memory>
#include "mncs_ueconnection.hpp"

namespace mncs {
bool Listener::startListener()
{
  _socket = socket(AF_INET, SOCK_STREAM, 0);

  if (_socket == -1) {
    logging::MultithreadPresets::defaultError("Couldn't init server socket\n");
    return false;
  }

  sockaddr_in server_addr{
      .sin_family = AF_INET, .sin_port = htons(_port), .sin_addr{}, .sin_zero{}};

  server_addr.sin_addr.s_addr = INADDR_ANY;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
    _socket = -1;
    logging::MultithreadPresets::defaultError("Couldn't init server socket\n");
    return false;
  }

  if (listen(_socket, kListenNumber) == -1) {
    logging::MultithreadPresets::defaultError("Couldn't init server socket\n");
    _socket = -1;
    return false;
  }
  return true;
}

void Listener::server()
{

  std::vector<size_t> vec;
  int client_socket{};
  auto start_connection = _connections.begin();
  while (!_shouldClose) {
    size_t counter{0};

    client_socket = accept(_socket, nullptr, nullptr);

    if (client_socket == -1) {
      logging::MultithreadPresets::defaultError("Couldn't init client socket\n");
      return;
    }
    _connections.emplace_back(std::make_unique<mncs::UEConnection>(
        _base_station_list, connection_id++, client_socket));

    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
      if (it->get()->isEnded()) {
        vec.push_back(it - start_connection);
      }
    }
    for (auto& idx : vec) {
      _connections.erase(_connections.begin() + static_cast<int64_t>(idx - counter++));
    }
  }
}
}  // namespace mncs

#include "mncs_listener.hpp"

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
  while (_shouldClose) {
    int client_socket{};

    client_socket = accept(_socket, nullptr, nullptr);

    if (client_socket == -1) {
      logging::MultithreadPresets::defaultError("Couldn't init client socket\n");
      return;
    }
    _connections.push_back(std::move({}));
  }
}
}  // namespace mncs

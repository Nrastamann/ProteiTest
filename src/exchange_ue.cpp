#include "exchange_ue.hpp"
#include <sys/socket.h>
#include "ip_addr.hpp"
#include "logger.hpp"
#include "resources_test.hpp"

namespace ue {
ConnectionStatus Exchanger::initConnection()
{
  _socket = socket(AF_INET, SOCK_STREAM, 0);
  if (_socket == -1) {
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't create socket to {}", _ip_addr));
    _in_active = false;
    return ConnectionStatus::socket_creation_err;
  }

  sockaddr_in server_addr{.sin_family = AF_INET,
                          .sin_port = htons(_ip_addr._port),
                          .sin_addr{_ip_addr.addrToNetwork()},
                          .sin_zero{0}};
  //NOLINTNEXTLINE
  int res = connect(_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

  if (res != 0) {
    _in_active = false;
    close(_socket);
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't connect to {}", _ip_addr));
    return ConnectionStatus::connection_err;
  }
  return ConnectionStatus::valid_connection;
}
}  // namespace ue

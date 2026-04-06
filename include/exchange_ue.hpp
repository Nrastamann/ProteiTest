#pragma once
#include <atomic>
#include "ip_addr.hpp"
#include "ue_context.hpp"
namespace ue {
enum class ConnectionStatus : uint8_t { socket_creation_err, connection_err, valid_connection };
class Exchanger {
 public:
  void setActive(bool value)
  {
    _in_active = value;
    if (_in_active) {
      initConnection();
    }
  }
  ConnectionStatus initConnection();
  ConnectionStatus checkRadiochannel();
  ConnectionStatus sendSms();
  ConnectionStatus getSms();

 private:
  UeContext _ctxt;
  network_addr::IpAddr _ip_addr;
  int _socket;
  std::atomic<bool> _in_active;
};
}  // namespace ue

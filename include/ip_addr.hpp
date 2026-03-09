#pragma once
#include <array>
#include <cstdint>
#include <format>

inline constexpr size_t kIpAddrOctetAmount{4};

struct IpAddr {
  std::array<uint8_t, kIpAddrOctetAmount> _addr;
  size_t _port;

  friend std::ostream& operator<<(std::ostream& stream, const IpAddr& addr);
};
template <>
struct std::formatter<IpAddr> : std::formatter<std::string> {
  auto format(const IpAddr& addr, std::format_context& ctx) const
  {
    std::string out;
    uint8_t last_octet = addr._addr.at(addr._addr.size() - 1);
    for (const auto& i : addr._addr) {
      char delimeter = i != last_octet ? '.' : ':';
      out += std::format("{}{}", i, delimeter);
    }
    out += std::format("{}", addr._port);
    return std::formatter<std::string>::format(out, ctx);
  }
};

#pragma once
#include <array>
#include <cstdint>
#include <format>

namespace network_addr {
inline constexpr size_t kIpAddrOctetAmount{4};

struct IpAddr {
 private:
  static constexpr uint8_t kSecondByteShift = 8;
  static constexpr uint8_t kThirdByteShift = 16;
  static constexpr uint8_t kFourthByteShift = 24;

 public:
  std::array<uint8_t, kIpAddrOctetAmount> _addr;
  uint16_t _port;

  [[nodiscard]] uint32_t addrToNetwork() const
  {
    return static_cast<uint32_t>(_addr[3] << kFourthByteShift | _addr[2] << kThirdByteShift |
                                 _addr[1] << kSecondByteShift | _addr[0]);
  }
  friend std::ostream& operator<<(std::ostream& stream, const IpAddr& addr)
  {
    stream << std::format("{}.{}.{}.{}:{}", addr._addr[0], addr._addr[1], addr._addr[2],
                          addr._addr[3], addr._port);

    return stream;
  }
};
}  // namespace network_addr

template <>
struct std::formatter<network_addr::IpAddr> : std::formatter<std::string> {
  auto format(const network_addr::IpAddr& addr, std::format_context& ctx) const
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

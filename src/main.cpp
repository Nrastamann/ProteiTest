#include "main.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>

constexpr size_t kIpAddrOctetAmount{4};
constexpr size_t kIpAddrOctetSize{3};

class Settings {
  std::string _lib_name;
  std::string _role;
  std::array<uint8_t, kIpAddrOctetAmount> _ip_addr{};
  size_t _port{};
  size_t _index{};

 public:
  [[nodiscard]] std::array<uint8_t, kIpAddrOctetAmount> const&
  getAddr() const
  {
    return _ip_addr;
  }
  [[nodiscard]] std::string const&
  getLibName() const
  {
    return _lib_name;
  }
  [[nodiscard]] std::string const&
  getRole() const
  {
    return _role;
  }
  [[nodiscard]] size_t const&
  getPort() const
  {
    return _port;
  }
  [[nodiscard]] size_t const&
  getIndex() const
  {
    return _index;
  }
  void
  setAddr(std::array<uint8_t, kIpAddrOctetAmount> const& ip_addr)
  {
    _ip_addr = ip_addr;
  }

  void
  setLibName(std::string_view lib_name)
  {
    _lib_name = lib_name;
  }
  void
  setRole(std::string_view role)
  {
    _role = role;
  }
  void
  setPort(size_t port)
  {
    _port = port;
  }
  void
  setIndex(size_t index)
  {
    _index = index;
  }

  ~Settings()                          = default;
  Settings()                           = default;
  Settings(Settings const&)            = default;
  Settings(Settings&&)                 = default;
  Settings& operator=(Settings const&) = default;
  Settings& operator=(Settings&&)      = default;
};

static bool
parseAddr(std::string_view ip_addr, Settings& settings)
{
  std::array<uint8_t, kIpAddrOctetAmount> addr{0};

  size_t octet_number  = 0;
  size_t len           = 0;
  auto const* it_begin = ip_addr.begin();
  for (auto const* it = ip_addr.begin(); it != ip_addr.end(); ++it) {
    if (octet_number >= kIpAddrOctetAmount || len > kIpAddrOctetSize) {
      return false;
    }

    bool is_end = it + 1 == ip_addr.end();

    if (*it != '.' && !is_end) {
      len++;
      continue;
    }

    len            += is_end ? 1 : 0;
    uint8_t octet   = 0;

    auto [ptr, ec]  = std::from_chars(it_begin, it_begin + len, octet);

    if (ec == std::errc::invalid_argument) {
      std::cout << " This is not a number.\n";
      return false;
    }
    if (ec == std::errc::result_out_of_range) {
      std::cout << "This number is larger than an int.\n";
      return false;
    }

    addr[octet_number++] = octet;
    it_begin             = ptr + 1;
    len                  = 0;
  }
  settings.setAddr(addr);
  return true;
}

inline static bool
parsePort(std::string_view port, Settings& settings)
{
  try {
    settings.setPort(std::stoull(port.data()));
    return true;
  }
  catch (std::exception& e) {
    return false;
  }
}

inline static bool
parseIndex(std::string_view index, Settings& settings)
{
  try {
    settings.setIndex(std::stoull(index.data()));
    return true;
  }
  catch (std::exception& e) {
    return false;
  }
}

size_t const kAddrHash  = {std::hash<std::string_view>{}("-a")};
size_t const kPortHash  = {std::hash<std::string_view>{}("-p")};
size_t const kRoleHash  = {std::hash<std::string_view>{}("-r")};
size_t const kIndexHash = {std::hash<std::string_view>{}("-i")};
size_t const kLibHash   = {std::hash<std::string_view>{}("-L")};

int
main(int argc, char* argv[])
{
  std::unordered_map<size_t, std::string_view> cl_args{
      { kAddrHash, "127.0.0.1"},
      { kPortHash,      "5555"},
      { kRoleHash,    "Client"},
      {kIndexHash,         "0"},
      {  kLibHash,      "Lib1"},
  };

  Settings command_line_options;
  int count = 1;
  while (count < argc) {
    auto it = cl_args.find(std::hash<std::string_view>{}(argv[count]));

    if (it == cl_args.end()) {
      std::cerr << "No such flag\n";
      return 0;
    }

    if (++count >= argc) {
      std::cerr << "Missed argument\n";
      return 0;
    }

    it->second = argv[count];
    count++;
  }

  if (!parseAddr(cl_args.at(kAddrHash), command_line_options)) {
    std::cout << "Invalid IP address\n";
    return 0;
  }
  if (!parsePort(cl_args.at(kPortHash), command_line_options)) {
    std::cout << "Invalid port\n";
    return 0;
  }
  if (!parseIndex(cl_args.at(kIndexHash), command_line_options)) {
    std::cout << "Invalid index\n";
    return 0;
  }
  command_line_options.setLibName(cl_args.at(kLibHash));
  command_line_options.setRole(cl_args.at(kRoleHash));
}

#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include "resources_test.hpp"

#include "hashed_values.hpp"
#include "static_containers.hpp"

inline constexpr size_t kIpAddrOctetAmount{4};
struct CommandLineArgsHolder {
 private:
  using IpAddressesHolder = std::vector<std::string_view>;

 public:
  IpAddressesHolder _addresses;
  std::vector<std::string_view> _lib_names;
  std::vector<std::string_view> _resource_names;

  std::string_view _port = "4444";
  std::string_view _role = "User";
  std::string_view _index = "0";

  [[nodiscard]] bool setArgument(size_t hash, std::string_view value);

 private:
  void addLib(std::string_view sv) { _lib_names.push_back(sv); }
  void addRole(std::string_view sv) { _role = sv; }
  void addPort(std::string_view sv) { _port = sv; }
  void addIndex(std::string_view sv) { _index = sv; }
  void addAddress(std::string_view sv) { _addresses.push_back(sv); }
};

class AppSettings {
  std::vector<std::string_view> _lib_name;
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> _ip_addr;

  std::string _role;
  std::string _name;
  size_t _type_hash;
  static_containers::EnumTypes _type_enum;

  size_t _port{};
  size_t _index{};
  bool _should_close = false;

 public:
  AppSettings(
      size_t port, size_t index, std::vector<std::string_view>&& lib_names,
      std::vector<std::array<unsigned char, kIpAddrOctetAmount>> addresses,
      std::string_view role, std::string&& userName = "UserName",
      size_t type_hash = hashed::kIntHash,
      static_containers::EnumTypes type_enum =
          static_containers::EnumTypes::Int)
      : _lib_name(std::move(lib_names)),
        _ip_addr(std::move(addresses)),
        _role(role),
        _name(std::move(userName)),
        _type_hash(type_hash),
        _type_enum(type_enum),
        _port(port),
        _index(index)
  {
    _should_close =
        !(ResourceTest{_lib_name}() && ConnectionTest{_ip_addr, port}());
  }
  [[nodiscard]] std::vector<std::array<uint8_t, kIpAddrOctetAmount>>& getAddr()
  {
    return _ip_addr;
  }

  [[nodiscard]] std::vector<std::string_view> const& cGetLibName() const
  {
    return _lib_name;
  }
  [[nodiscard]] size_t cgetTypeHash() const { return _type_hash; }
  [[nodiscard]] static_containers::EnumTypes cgetTypeEnum() const
  {
    return _type_enum;
  }
  [[nodiscard]] std::string_view cgetName() const { return _name; }
  [[nodiscard]] std::string_view cgetRole() const { return _role; }
  [[nodiscard]] size_t cgetPort() const { return _port; }
  [[nodiscard]] size_t cgetIndex() const { return _index; }
  [[nodiscard]] bool cgetShouldClose() const { return _should_close; }
  [[nodiscard]] const std::vector<std::array<uint8_t, kIpAddrOctetAmount>>&
  cgetAddress() const
  {
    return _ip_addr;
  }

  void setShouldClose() { _should_close = true; }
  void setName(std::string&& str) { _name = std::move(str); }

  void setTypeHash(size_t hash) { _type_hash = hash; }
  void setTypeEnum(static_containers::EnumTypes type) { _type_enum = type; }

  ~AppSettings() = default;
  AppSettings() = delete;
  AppSettings(AppSettings const&) = default;
  AppSettings(AppSettings&&) = default;
  AppSettings& operator=(AppSettings const&) = default;
  AppSettings& operator=(AppSettings&&) = default;
};

namespace ui_protei {
void printAppSettings(AppSettings const& settings);
}  // namespace ui_protei

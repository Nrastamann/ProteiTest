#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "resources_test.hpp"

#include "hashed_values.hpp"
#include "static_containers.hpp"

inline constexpr size_t kIpAddrOctetAmount{4};
struct CommandLineArgsHolder {
 private:
  using array_type = std::vector<std::string_view>;

 public:
  array_type _addresses;
  array_type _lib_names;
  array_type _resource_names;
  array_type _ports;

  std::string_view _role = "User";
  std::string_view _index = "0";

  [[nodiscard]] bool setArgument(size_t hash, std::string_view value);

 private:
  void addLib(std::string_view sv) { _lib_names.push_back(sv); }
  void addRole(std::string_view sv) { _role = sv; }
  void addPort(std::string_view sv) { _ports.push_back(sv); }
  void addIndex(std::string_view sv) { _index = sv; }
  void addAddress(std::string_view sv) { _addresses.push_back(sv); }
};

class AppSettings {
  std::vector<std::string_view> _lib_name;
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> _ip_addr;
  std::vector<size_t> _ports;

  std::string _role;
  std::string _name;
  size_t _type_hash;
  static_containers::EnumTypes _type_enum;

  size_t _index{};
  bool _should_close = false;

 public:
  AppSettings(
      std::vector<size_t> ports, std::vector<std::string_view> lib_names,
      std::vector<std::array<unsigned char, kIpAddrOctetAmount>> addresses,
      std::string_view role, size_t index, std::string&& userName = "UserName",
      size_t type_hash = hashed::kInt,
      static_containers::EnumTypes type_enum =
          static_containers::EnumTypes::Int)
      : _lib_name(std::move(lib_names)),
        _ip_addr(std::move(addresses)),
        _ports(std::move(ports)),
        _role(role),
        _name(std::move(userName)),
        _type_hash(type_hash),
        _type_enum(type_enum),
        _index(index)
  {
    _should_close =
        !(ResourceTest{_lib_name}() && ConnectionTest{_ip_addr, _ports}());
  }
  [[nodiscard]] std::vector<std::array<uint8_t, kIpAddrOctetAmount>>& getAddr()
  {
    return _ip_addr;
  }
  [[nodiscard]] const std::vector<size_t>& cgetPort() const { return _ports; }
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

namespace hashed {
inline size_t const kAddrHash = {std::hash<std::string_view>{}("-a")};
inline size_t const kPortHash = {std::hash<std::string_view>{}("-p")};
inline size_t const kRoleHash = {std::hash<std::string_view>{}("-r")};
inline size_t const kIndexHash = {std::hash<std::string_view>{}("-i")};
inline size_t const kLibHash = {std::hash<std::string_view>{}("-L")};
}  // namespace hashed

namespace ui_protei {
void printAppSettings(AppSettings const& settings);
}  // namespace ui_protei

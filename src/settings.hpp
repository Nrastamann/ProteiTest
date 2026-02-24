#pragma once
#include <array>
#include <cstdint>
#include <ostream>
#include <string>

#include "hashed_values.hpp"
#include "static_containers.hpp"

inline constexpr size_t kIpAddrOctetAmount{4};

class Settings {
  std::string _lib_name;
  std::string _role;
  std::array<uint8_t, kIpAddrOctetAmount> _ip_addr{};
  size_t _type_hash{hashed::kIntHash};
  static_containers::EnumTypes _type_enum{static_containers::EnumTypes::Int};
  size_t _port{};
  size_t _index{};

 public:
  [[nodiscard]] std::array<uint8_t, kIpAddrOctetAmount> const& getAddr() const
  {
    return _ip_addr;
  }

  [[nodiscard]] std::string const& cGetLibName() const { return _lib_name; }
  [[nodiscard]] size_t cgetTypeHash() const { return _type_hash; }
  [[nodiscard]] static_containers::EnumTypes cgetTypeEnum() const
  {
    return _type_enum;
  }
  [[nodiscard]] std::string const& cgetRole() const { return _role; }
  [[nodiscard]] size_t cgetPort() const { return _port; }
  [[nodiscard]] size_t cgetIndex() const { return _index; }
  void setAddr(std::array<uint8_t, kIpAddrOctetAmount> const& ip_addr)
  {
    _ip_addr = ip_addr;
  }
  void setTypeHash(size_t hash) { _type_hash = hash; }
  void setTypeEnum(static_containers::EnumTypes type) { _type_enum = type; }
  void setLibName(std::string_view lib_name) { _lib_name = lib_name; }
  void setRole(std::string_view role) { _role = role; }
  void setPort(size_t port) { _port = port; }
  void setIndex(size_t index) { _index = index; }
  friend std::ostream& operator<<(std::ostream& out, Settings const& settings);

  ~Settings() = default;
  Settings() = default;
  Settings(Settings const&) = default;
  Settings(Settings&&) = default;
  Settings& operator=(Settings const&) = default;
  Settings& operator=(Settings&&) = default;
};

std::ostream& operator<<(std::ostream& out, Settings const& settings);

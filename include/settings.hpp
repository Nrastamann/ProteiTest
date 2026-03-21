#pragma once
#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "custom_types.hpp"
#include "ip_addr.hpp"
#include "logger.hpp"
#include "resources_test.hpp"

/**
 * class AppSettings - Class to hold settings during runtime 
 *
 * If error occures during checking resources (sockets or files) _should_close member is set to true
 * and program ends
 *
 * If there's no error during getting resources - _should_close sets to false
 */
class AppSettings {

  std::vector<std::string> _lib_name;
  std::vector<network_addr::IpAddr> _addresses;

  std::string _role{};
  std::string _name{};
  size_t _type_hash{};
  custom_types::EnumTypes _type_enum{};

  size_t _index{};
  bool _should_close = false;

 public:
  AppSettings(
      std::vector<uint16_t> ports, std::vector<std::string> lib_names,
      std::vector<std::array<unsigned char, network_addr::kIpAddrOctetAmount>> addresses,
      std::string_view role, size_t index, std::string&& userName = "UserName",
      size_t type_hash = hashed::kInt,
      custom_types::EnumTypes type_enum = custom_types::EnumTypes::Int)
      : _lib_name(std::move(lib_names)),
        _role(role),
        _name(std::move(userName)),
        _type_hash(type_hash),
        _type_enum(type_enum),
        _index(index)
  {
    if (addresses.size() != ports.size()) {
      logging::SingleThreadPresets::defaultError("Adresses number not equal ports");
      _should_close = true;
      return;
    }
    for (size_t i = 0; i < addresses.size(); ++i) {
      _addresses.push_back({addresses[i], ports[i]});
    }

    logging::SingleThreadPresets::createObject<resources_tests::ResourceTest>();
    logging::SingleThreadPresets::createObject<resources_tests::ConnectionTest>();

    _should_close = !(resources_tests::ResourceTest{_lib_name}() &&
                      resources_tests::ConnectionTest{_addresses}());
  }
  [[nodiscard]] std::vector<network_addr::IpAddr>& getAddr() { return _addresses; }
  [[nodiscard]] std::vector<std::string> const& cGetLibName() const { return _lib_name; }
  [[nodiscard]] size_t cgetTypeHash() const { return _type_hash; }
  [[nodiscard]] custom_types::EnumTypes cgetTypeEnum() const { return _type_enum; }
  [[nodiscard]] std::string_view cgetName() const { return _name; }
  [[nodiscard]] std::string_view cgetRole() const { return _role; }
  [[nodiscard]] size_t cgetIndex() const { return _index; }
  [[nodiscard]] bool cgetShouldClose() const { return _should_close; }
  [[nodiscard]] const std::vector<network_addr::IpAddr>& cgetAddress() const
  {
    return _addresses;
  }

  void setShouldClose() { _should_close = true; }
  template <typename T>
  void setName(T&& str)
  {
    _name = std::forward<T>(str);
  }

  void setTypeHash(size_t hash) { _type_hash = hash; }
  void setTypeEnum(custom_types::EnumTypes type) { _type_enum = type; }

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

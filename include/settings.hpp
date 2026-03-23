#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "custom_types.hpp"
#include "ip_addr.hpp"
#include "logger.hpp"
#include "parsing.hpp"
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
 public:
  explicit AppSettings(parsing::ArgHolder& arguments)
      : _lib_name(arguments.getLibs()),
        _addresses(arguments.getAddr()),
        _role(arguments.getRole()),
        _index(arguments.getIndex())
  {
    logging::SingleThreadPresets::createObject<resources_tests::ResourceTest>();
    logging::SingleThreadPresets::createObject<resources_tests::ConnectionTest>();

    _should_close = !(resources_tests::ResourceTest{_lib_name}() &&
                      resources_tests::ConnectionTest{_addresses}());
  }
  [[nodiscard]] std::vector<network_addr::IpAddr>& getAddr() { return _addresses; }
  [[nodiscard]] std::vector<std::string> const& cGetLibName() const { return _lib_name; }
  [[nodiscard]] size_t cgetTypeHash() const { return _type_hash; }
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

 private:
  std::vector<std::string> _lib_name;
  std::vector<network_addr::IpAddr> _addresses;

  std::string _role;
  std::string _name{"UserName"};

  size_t _type_hash{hashed::kInt};
  size_t _index;

  bool _should_close{false};
};

namespace ui_protei {
void printAppSettings(AppSettings const& settings);
}  // namespace ui_protei

#pragma once
#include <vector>
#include "ip_addr.hpp"
#include "logger.hpp"
#include "parsing.hpp"
#include "resources_test.hpp"

class AppSettings {
 public:
  explicit AppSettings(parsing::ArgHolder& arguments)
      : _addresses(arguments.getAddr()), _index(arguments.getIndex())
  {
    logging::SingleThreadPresets::createObject<resources_tests::ConnectionTest>();

    _should_close = !resources_tests::ConnectionTest{_addresses}();
  }
  [[nodiscard]] std::vector<network_addr::IpAddr>& getAddr() { return _addresses; }
  [[nodiscard]] size_t cgetIndex() const { return _index; }
  [[nodiscard]] bool cgetShouldClose() const { return _should_close; }
  [[nodiscard]] const std::vector<network_addr::IpAddr>& cgetAddress() const
  {
    return _addresses;
  }

  void setShouldClose() { _should_close = true; }

 private:
  std::vector<network_addr::IpAddr> _addresses;
  size_t _index;
  bool _should_close{false};
};

namespace ui_protei {
void printAppSettings(AppSettings const& settings);
}  // namespace ui_protei

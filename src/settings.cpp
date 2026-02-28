#include "settings.hpp"
#include <iostream>

namespace ui_protei {

void printSettings(Settings const& settings)
{
  std::cout << "========================\n";

  std::cout << "Current settings:\n";

  std::cout << "Role:\t\t" << settings.cgetRole() << '\n';
  std::cout << "Index:\t\t" << settings.cgetIndex() << '\n';
  std::cout << "IP address:\t";
  size_t delimeter_index = 0;

  for (const auto& octet : settings.cgetAddress()) {
    char delimeter = ++delimeter_index < kIpAddrOctetAmount ? '.' : ' ';
    std::cout << +octet << delimeter;
  }

  std::cout << '\n';
  std::cout << "Port:\t\t" << settings.cgetPort() << '\n';
  std::cout << "Library name:\t" << settings.cGetLibName() << '\n';
  std::cout << "Current type:\t"
            << static_containers::getImplementedTypes().at(
                   settings.cgetTypeHash())
            << '\n';
  std::cout << "========================\n";
}
}  // namespace ui_protei

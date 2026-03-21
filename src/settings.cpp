#include "settings.hpp"

#include <string_view>

#include "custom_types.hpp"
#include "logger.hpp"
namespace ui_protei {

void printAppSettings(AppSettings const& settings)
{
  logging::SingleThreadPresets::functionCall();

  std::cout << "========================\n";
  std::cout << "Current settings:\n";
  std::cout << "UserName:\t" << settings.cgetName() << '\n';

  std::cout << "Role:\t\t" << settings.cgetRole() << '\n';
  std::cout << "Index:\t\t" << settings.cgetIndex() << '\n';
  std::cout << "IP address:\n";
  for (const auto& address : settings.cgetAddress()) {
    std::cout << '\t' << address << '\n';
  }

  std::cout << '\n';
  std::cout << "Library names:\n";
  for (const auto& lib : settings.cGetLibName()) {
    std::cout << '\t' << lib << '\n';
  }

  std::cout << "Current type:\t"
            << custom_types::getHashToTypeInfo().at(settings.cgetTypeHash()).second << '\n';
  std::cout << "========================\n";
}

}  // namespace ui_protei

#include "settings.hpp"

#include "logger.hpp"
namespace ui_protei {

void printAppSettings(AppSettings const& settings)
{
  logging::SingleThreadPresets::functionCall();

  std::cout << "========================\n";
  std::cout << "Current settings:\n";

  std::cout << "IP address:\n";
  for (const auto& address : settings.cgetAddress()) {
    std::cout << '\t' << address << '\n';
  }

  std::cout << '\n';
  std::cout << "Library names:\n";
  std::cout << "========================\n";
}

}  // namespace ui_protei

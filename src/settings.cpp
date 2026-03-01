#include "settings.hpp"
#include <iostream>

namespace ui_protei {

void printAppSettings(AppSettings const& settings)
{
  std::cout << "========================\n";
  std::cout << "Current settings:\n";
  std::cout << "UserName:\t" << settings.cgetName() << '\n';

  std::cout << "Role:\t\t" << settings.cgetRole() << '\n';
  std::cout << "Index:\t\t" << settings.cgetIndex() << '\n';
  std::cout << "IP address:\t";
  size_t delimeter_index = 0;

  for (const auto& addr : settings.cgetAddress()) {
    std::cout << '\t';
    for (const auto& octet : addr) {
      char delimeter = ++delimeter_index < kIpAddrOctetAmount ? '.' : ' ';
      std::cout << +octet << delimeter;
    }
    delimeter_index = 0;
    std::cout << "\n";
  }

  std::cout << '\n';
  std::cout << "Port:\t\t" << settings.cgetPort() << '\n';
  std::cout << "Library names:\n";
  for (const auto& lib : settings.cGetLibName()) {
    std::cout << '\t' << lib << '\n';
  }

  std::cout << "Current type:\t"
            << static_containers::getImplementedTypes().at(
                   settings.cgetTypeHash())
            << '\n';
  std::cout << "========================\n";
}

}  // namespace ui_protei

bool CommandLineArgsHolder::setArgument(size_t hash, std::string_view value)
{
  static std::unordered_map<size_t, std::function<void(std::string_view)>>
      cl_args = {
          {hashed::kAddrHash,
           [&arg_holder = *this](std::string_view sv) {
             arg_holder.addAddress(sv);
           }},
          {hashed::kPortHash,
           [&arg_holder = *this](std::string_view sv) {
             arg_holder.addPort(sv);
           }},
          {hashed::kRoleHash,
           [&arg_holder = *this](std::string_view sv) {
             arg_holder.addRole(sv);
           }},
          {hashed::kIndexHash,
           [&arg_holder = *this](std::string_view sv) {
             arg_holder.addIndex(sv);
           }},
          {hashed::kLibHash,
           [&arg_holder = *this](std::string_view sv) {
             arg_holder.addLib(sv);
           }},
      };

  auto it = cl_args.find(hash);
  bool return_value = it != cl_args.end();

  return_value ? (it->second)(value) : void();

  return return_value;
}

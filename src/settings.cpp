#include "settings.hpp"
#include "io_manager.hpp"
#include "static_containers.hpp"

namespace ui_protei {

void printAppSettings(AppSettings const& settings)
{
  auto& out = protei_io::io().cout();
  logger_presets::functionCall();

  out << "========================\n";
  out << "Current settings:\n";
  out << "UserName:\t" << settings.cgetName() << '\n';

  out << "Role:\t\t" << settings.cgetRole() << '\n';
  out << "Index:\t\t" << settings.cgetIndex() << '\n';
  out << "IP address:\n";
  size_t delimeter_index = 0;

  for (const auto& addr : settings.cgetAddress()) {
    out << '\t';
    for (const auto& octet : addr) {
      char delimeter = ++delimeter_index < kIpAddrOctetAmount ? '.' : ' ';
      out << +octet << delimeter;
    }
    delimeter_index = 0;
    out << "\n";
  }

  out << '\n';
  out << "Port:\n";
  for (const auto& port : settings.cgetPort()) {
    out << '\t' << port << '\n';
  }
  out << "Library names:\n";
  for (const auto& lib : settings.cGetLibName()) {
    out << '\t' << lib << '\n';
  }

  out << "Current type:\t"
      << static_containers::getHashToTypeInfo()
             .at(settings.cgetTypeHash())
             .second
      << '\n';
  out << "========================\n";
}

}  // namespace ui_protei

bool CommandLineArgsHolder::setArgument(size_t hash, std::string_view value)
{
  logger_presets::functionCall();

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

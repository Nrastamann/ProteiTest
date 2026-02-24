#include "settings.hpp"

std::ostream& operator<<(std::ostream& out, Settings const& settings)
{
  out << "========================\n";

  out << "Current settings:\n";

  out << "Role:\t\t" << settings._role << '\n';
  out << "Index:\t\t" << settings._index << '\n';
  out << "IP address:\t";
  size_t delimeter_index = 0;

  for (const auto& octet : settings._ip_addr) {
    char delimeter = ++delimeter_index < settings._ip_addr.size() ? '.' : ' ';
    out << +octet << delimeter;
  }

  out << '\n';
  out << "Port:\t\t" << settings._port << '\n';
  out << "Library name:\t" << settings._lib_name << '\n';
  out << "Current type:\t"
      << static_containers::getImplementedTypes().at(settings._type_hash)
      << '\n';
  out << "========================\n";
  return out;
}

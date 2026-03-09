#include "settings.hpp"
#include <array>
#include <expected>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>

#include "logger.hpp"
#include "parsing.hpp"
#include "static_containers.hpp"

std::vector<size_t> CommandLineArgsHolder::getPorts()
{
  using port_parse_result = std::expected<size_t, parsing_protei::ParseResult>;
  namespace rv = std::ranges::views;
  std::vector<size_t> ports_arr;

  auto ports_view = _ports | rv::transform(&parsing_protei::parsePort) |
                    rv::take_while(&port_parse_result::has_value) |
                    rv::transform([](const auto& a) { return a.value(); });

  for (const auto& i : ports_view) {
    ports_arr.push_back(i);
  }

  if (ports_arr.size() != _ports.size()) {
    _error_parsing = true;
    return {};
  }

  return ports_arr;
}

std::vector<std::array<uint8_t, kIpAddrOctetAmount>>
CommandLineArgsHolder::getAddresses()
{
  namespace rv = std::ranges::views;
  using addr_parse_result =
      std::expected<std::array<uint8_t, kIpAddrOctetAmount>,
                    parsing_protei::ParseResult>;
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> ip_arr;

  auto addresses_view = _addresses | rv::transform(&parsing_protei::parseAddr) |
                        rv::take_while(&addr_parse_result::has_value) |
                        rv::transform([](const auto& a) { return a.value(); });

  for (const auto& i : addresses_view) {
    ip_arr.push_back(i);
  }

  if (ip_arr.size() != _addresses.size()) {
    _error_parsing = true;
    return {};
  }

  return ip_arr;
}

size_t CommandLineArgsHolder::getIndex()
{
  std::expected<size_t, parsing_protei::ParseResult> index =
      parsing_protei::parseIndex(_index);

  if (!index.has_value()) {
    logger_presets::defaultError("Couldn't parse the index");
    _error_parsing = true;
    return 0;
  }
  return index.value();
}

namespace ui_protei {

void printAppSettings(AppSettings const& settings)
{
  logger_presets::functionCall();

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
            << static_containers::getHashToTypeInfo()
                   .at(settings.cgetTypeHash())
                   .second
            << '\n';
  std::cout << "========================\n";
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

static void joinAddr(std::vector<std::string>& result_vector,
                     std::string_view ip_addr)
{
  logger_presets::functionCall();
  std::string_view digits = "0123456789abcdef";
  std::string res_str;

  size_t substr_begin = 0;

  for (size_t i = 0; i < 4; ++i) {
    size_t end = ip_addr.find_first_not_of(digits, substr_begin);
    bool is_last_octet = i == 3;
    if (end == std::string::npos && !is_last_octet) {
      break;
    }

    res_str += ip_addr.substr(substr_begin, end - substr_begin);

    ip_addr.remove_prefix(ip_addr.size() < end ? ip_addr.size() - 1 : end);
    substr_begin = ip_addr.find_first_of(digits);

    if (substr_begin == std::string::npos || is_last_octet) {
      break;
    }
    res_str += '.';
  }
  result_vector.push_back(std::move(res_str));

  if (ip_addr.size() != 1) {
    result_vector.emplace_back("-p");

    size_t end = ip_addr.find_first_not_of(digits, substr_begin);

    result_vector.emplace_back(
        ip_addr.substr(substr_begin, end - substr_begin));
  }
}

std::vector<std::string> getInput(char** argv, int argc)
{
  logger_presets::functionCall();
  std::vector<std::string> returning_vector;
  auto span_args = std::span(argv, static_cast<size_t>(argc));
  span_args = span_args.subspan(1);
  bool parsing_addr = false;

  std::string ip_addr;
  argc -= 1;

  for (auto& sv : span_args) {
    if (parsing_addr && ('-' != *sv || argc == 0)) {
      ip_addr += std::format("{} ", sv);

      if (argc != 0) {
        continue;
      }
    }

    parsing_addr = false;
    if (ip_addr.length() != 0) {
      joinAddr(returning_vector, ip_addr);
      ip_addr = "";
      if (argc == 0)
        break;
    }

    size_t text_hash = std::hash<std::string_view>{}(sv);

    if (hashed::kAddrHash == text_hash || hashed::kAddrBigHash == text_hash) {
      parsing_addr = true;
    }

    returning_vector.emplace_back(sv);
  }
  return returning_vector;
}

std::ostream& operator<<(std::ostream& stream, const IpAddr& addr)
{
  stream << std::format("{}.{}.{}.{}:{}", addr._addr[0], addr._addr[1],
                        addr._addr[2], addr._addr[3], addr._port);

  return stream;
}

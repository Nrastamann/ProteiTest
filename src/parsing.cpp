#include "parsing.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <expected>
#include <format>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include "logger.hpp"
#include "settings.hpp"
static constexpr size_t kHexBase{16};

namespace parsing_protei {
std::expected<std::array<uint8_t, kIpAddrOctetAmount>, ParseResult> parseAddr(
    std::string_view ip_addr)
{

  bool has_hex = ip_addr.find_first_of("abcdef") != std::string::npos;
  logger_presets::functionCall();
  std::array<uint8_t, kIpAddrOctetAmount> addr{0};

  for (auto& octet : addr) {
    const size_t delimeter_pos = ip_addr.find('.');
    std::string_view substr_octet = ip_addr.substr(0, delimeter_pos);

    std::from_chars_result err_res =
        has_hex
            ? std::from_chars(substr_octet.begin(), substr_octet.end(), octet,
                              kHexBase)
            : std::from_chars(substr_octet.begin(), substr_octet.end(), octet);

    if (err_res.ec != std::errc() || err_res.ptr != substr_octet.end()) {
      logger_presets::userInputError(substr_octet, *err_res.ptr);
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }

    ip_addr.remove_prefix(delimeter_pos + 1);
  }

  return addr;
}

std::expected<uint16_t, ParseResult> parsePort(std::string_view port)
{
  logger_presets::functionCall();

  uint16_t port_number{};
  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_number);

  if (ec != std::errc() || ptr != port.end()) {
    logger_presets::userInputError(port, *ptr);

    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return port_number;
}

std::expected<size_t, ParseResult> parseIndex(std::string_view index)
{
  logger_presets::functionCall();

  size_t index_number{};
  auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);
  if (ec != std::errc() || ptr != index.end()) {
    logger_presets::userInputError(index, *ptr);
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return index_number;
}

std::expected<CommandLineArgsHolder, ParseResult> parseClArgs(
    std::span<std::string> vec)
{
  logger_presets::functionCall();

  CommandLineArgsHolder argument_holder{};
  bool is_next_arg = false;

  size_t hash = 0;
  for (auto& argument : vec) {
    if (is_next_arg) {
      bool is_valid_argument = argument_holder.setArgument(hash, argument);

      if (!is_valid_argument) {
        logger_presets::parsingInputError(argument, *argument.begin());
        return std::unexpected(ParseResult::WRONG_FLAG);
      }

      is_next_arg = !is_next_arg;
      continue;
    }

    std::ranges::transform(argument, argument.begin(), ::tolower);
    hash = std::hash<std::string_view>{}(argument);
    is_next_arg = !is_next_arg;
  }

  if (is_next_arg) {
    bool is_valid_argument = argument_holder.setArgument(hash, "");
    if (!is_valid_argument) {
      return std::unexpected(ParseResult::WRONG_FLAG);
    }
    logger_presets::defaultError(
        std::format("Last unpaired flag - {}", vec.last(1)));

    return std::unexpected(ParseResult::NO_ARGUMENT);
  }

  return argument_holder;
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
    argc--;
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
std::vector<uint16_t> CommandLineArgsHolder::getPorts()
{
  using port_parse_result = std::expected<uint16_t, ParseResult>;
  namespace rn = std::ranges;
  namespace rv = std::ranges::views;

  std::vector<uint16_t> ports_arr;

  auto ports_view = _ports | rv::transform(&parsePort) |
                    rv::take_while(&port_parse_result::has_value) |
                    rv::transform([](const auto& a) { return a.value(); });

  rn::copy(ports_view, ports_arr.begin());

  if (ports_arr.size() != _ports.size()) {
    _error_parsing = true;
    return {};
  }

  return ports_arr;
}

std::vector<std::array<uint8_t, kIpAddrOctetAmount>>
CommandLineArgsHolder::getAddresses()
{
  namespace rn = std::ranges;
  namespace rv = std::ranges::views;
  using addr_parse_result =
      std::expected<std::array<uint8_t, kIpAddrOctetAmount>, ParseResult>;
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> ip_arr;

  auto addresses_view = _addresses | rv::transform(&parseAddr) |
                        rv::take_while(&addr_parse_result::has_value) |
                        rv::transform([](const auto& a) { return a.value(); });

  rn::copy(addresses_view, ip_arr.begin());

  if (ip_arr.size() != _addresses.size()) {
    _error_parsing = true;
    return {};
  }

  return ip_arr;
}

size_t CommandLineArgsHolder::getIndex()
{
  std::expected<size_t, ParseResult> index = parseIndex(_index);

  if (!index.has_value()) {
    logger_presets::defaultError("Couldn't parse the index");
    _error_parsing = true;
    return 0;
  }
  return index.value();
}

};  // namespace parsing_protei

#include "parsing.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <expected>
#include <format>
#include <span>
#include "logger.hpp"
#include "settings.hpp"
static constexpr size_t kHexBase{16};

namespace parsing_protei {
std::expected<std::array<uint8_t, kIpAddrOctetAmount>, ParseResult> parseAddr(
    std::string_view ip_addr)
{
  logger_presets::functionCall();
  std::array<uint8_t, kIpAddrOctetAmount> addr{0};

  for (auto& octet : addr) {
    const size_t delimeter_pos = ip_addr.find('.');
    std::string_view substr_octet = ip_addr.substr(0, delimeter_pos);

    auto [ptr, ec] =
        std::from_chars(substr_octet.begin(), substr_octet.end(), octet);

    if (ec != std::errc() || ptr != substr_octet.end()) {
      auto [ptr_hex, ec_hex] = std::from_chars(
          substr_octet.begin(), substr_octet.end(), octet, kHexBase);

      if (ec_hex != std::errc() || ptr_hex != substr_octet.end()) {
        logger_presets::userInputError(substr_octet, *ptr);
        return std::unexpected(ParseResult::SV_PARSING_ERR);
      }
    }

    ip_addr.remove_prefix(delimeter_pos + 1);
  }

  return addr;
}

std::expected<size_t, ParseResult> parsePort(std::string_view port)
{
  logger_presets::functionCall();

  size_t port_number{};
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

};  // namespace parsing_protei

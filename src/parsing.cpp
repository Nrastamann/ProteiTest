#include "parsing.hpp"
#include <array>
#include <charconv>
#include <expected>
#include <span>
#include "settings.hpp"
namespace parsing_protei {
std::expected<std::array<uint8_t, kIpAddrOctetAmount>, ParseResult> parseAddr(
    std::string_view ip_addr)
{
  std::array<uint8_t, kIpAddrOctetAmount> addr{0};

  for (auto& octet : addr) {
    const size_t delimeter_pos = ip_addr.find('.');
    std::string_view substr_octet = ip_addr.substr(0, delimeter_pos);

    auto [ptr, ec] =
        std::from_chars(substr_octet.begin(), substr_octet.end(), octet);

    if (ec != std::errc() || ptr != substr_octet.end()) {
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }

    ip_addr.remove_prefix(delimeter_pos + 1);
  }

  return addr;
}

std::expected<size_t, ParseResult> parsePort(std::string_view port)
{
  size_t port_number{};
  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_number);

  if (ec != std::errc() || ptr != port.end()) {
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return port_number;
}

std::expected<size_t, ParseResult> parseIndex(std::string_view index)
{
  size_t index_number{};
  auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);

  if (ec != std::errc() || ptr != index.end()) {
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return index_number;
}

std::expected<CommandLineArgsHolder, ParseResult> parseClArgs(char** argv,
                                                              int argc)
{
  CommandLineArgsHolder argument_holder{};
  bool is_next_arg = false;
  auto argv_span = std::span(argv, argc).subspan(1);
  size_t hash = 0;

  for (auto& argument : argv_span) {
    if (is_next_arg) {
      bool is_valid_argument = argument_holder.setArgument(hash, argument);

      if (!is_valid_argument) {
        return std::unexpected(ParseResult::WRONG_FLAG);
      }

      is_next_arg = !is_next_arg;
      continue;
    }
    hash = std::hash<std::string_view>{}(argument);
    is_next_arg = !is_next_arg;
  }

  if (is_next_arg) {
    return std::unexpected(ParseResult::NO_ARGUMENT);
  }

  return argument_holder;
}

};  // namespace parsing_protei

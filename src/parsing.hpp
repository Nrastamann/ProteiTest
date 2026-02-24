#pragma once
#include <charconv>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include "settings.hpp"

enum class ParseResult : uint8_t {
  NO_ERR,
  WRONG_FLAG,
  NO_ARGUMENT,
};

inline bool parseAddr(std::string_view ip_addr, Settings& settings)
{
  std::array<uint8_t, kIpAddrOctetAmount> addr{0};

  for (auto& octet : addr) {
    const size_t delimeter_pos = ip_addr.find('.');
    std::string_view substr_octet = ip_addr.substr(0, delimeter_pos);

    auto [ptr, ec] =
        std::from_chars(substr_octet.begin(), substr_octet.end(), octet);

    if (ec != std::errc() || ptr != substr_octet.end()) {
      return false;
    }

    ip_addr.remove_prefix(delimeter_pos + 1);
  }

  settings.setAddr(addr);
  return true;
}

inline bool parsePort(std::string_view port, Settings& settings)
{
  size_t port_number{};
  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_number);
  if (ec != std::errc() || ptr != port.end()) {
    return false;
  }
  settings.setPort(port_number);
  return true;
}

inline bool parseIndex(std::string_view index, Settings& settings)
{
  size_t index_number{};
  auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);
  if (ec != std::errc() || ptr != index.end()) {
    return false;
  }
  settings.setPort(index_number);
  return true;
}

inline static ParseResult parseClArgs(
    std::unordered_map<size_t, std::string_view>& argument_map, char** argv,
    int argc)
{
  bool is_next_arg = false;
  bool first_arg = true;
  auto it = argument_map.begin();

  for (auto& argument : std::span(argv, argc)) {
    //need to skip program name as variable without warning about pointer
    //arithmetic, idk how else i can do this
    if (first_arg) {
      first_arg = false;
      continue;
    }

    if (is_next_arg) {
      is_next_arg = false;
      it->second = argument;
      continue;
    }

    it = argument_map.find(std::hash<std::string_view>{}(argument));

    if (it == argument_map.end()) {
      std::cout << argument << std::hash<std::string_view>{}(argument) << '\n';
      std::cerr << "No such flag\n";
      return ParseResult::WRONG_FLAG;
    }

    is_next_arg = true;
  }

  if (is_next_arg) {
    std::cerr << "Missed argument\n";
    return ParseResult::NO_ARGUMENT;
  }

  return ParseResult::NO_ERR;
}

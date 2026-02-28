#pragma once
#include <cstdint>
#include <string_view>
#include "settings.hpp"

enum class ParseResult : uint8_t {
  NO_ERR,
  WRONG_FLAG,
  NO_ARGUMENT,
};

[[nodiscard("Discarding address parsing result")]] bool parseAddr(
    std::string_view ip_addr, Settings& settings);
[[nodiscard("Discarding port parsing result")]] bool parsePort(
    std::string_view port, Settings& settings);
[[nodiscard("Discarding index parsing result")]] bool parseIndex(
    std::string_view index, Settings& settings);
[[nodiscard("Discarding cl_args parse_result")]] ParseResult parseClArgs(
    std::unordered_map<size_t, std::string_view>& argument_map, char** argv,
    int argc);

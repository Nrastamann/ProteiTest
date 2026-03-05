#pragma once
#include <cstdint>
#include <expected>
#include <string_view>
#include "settings.hpp"

namespace parsing_protei {
enum class ParseResult : uint8_t {
  NO_ERR,
  WRONG_FLAG,
  NO_ARGUMENT,
  SV_PARSING_ERR
};

[[nodiscard("Discarding address parsing result")]] std::expected<
    std::array<uint8_t, kIpAddrOctetAmount>, ParseResult>
parseAddr(std::string_view ip_addr);

[[nodiscard(
    "Discarding port parsing result")]] std::expected<size_t, ParseResult>
parsePort(std::string_view port);

[[nodiscard(
    "Discarding index parsing result")]] std::expected<size_t, ParseResult>
parseIndex(std::string_view index);

[[nodiscard("Discarding cl_args parse_result")]] std::expected<
    CommandLineArgsHolder, ParseResult>
parseClArgs(char** argv, int argc);
};  // namespace parsing_protei

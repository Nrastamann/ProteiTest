#pragma once
#include <cstdint>
#include <expected>
#include <string_view>
#include <vector>
#include "ip_addr.hpp"

namespace parsing_protei {
enum class ParseResult : uint8_t {
  NO_ERR,
  WRONG_FLAG,
  NO_ARGUMENT,
  SV_PARSING_ERR
};
/**
 * struct CommandLineArgsHolder - Struct to hold strings for future parsing 
 *
 * If error occures during parsing of ports/addr/index - _error_parsing is set to true,
 * so program ends
 *
 * If there's no error during parsing - _error_parsing sets to false
 */
struct CommandLineArgsHolder {
 private:
  using array_type = std::vector<std::string_view>;

 public:
  std::vector<uint16_t> getPorts();
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> getAddresses();
  size_t getIndex();
  std::string_view getRole() { return _role; }
  array_type& getLibs() { return _lib_names; }

  [[nodiscard]] bool parsingStatus() const { return _error_parsing; }

  [[nodiscard]] bool setArgument(size_t hash, std::string_view value);

 private:
  void addLib(std::string_view sv) { _lib_names.push_back(sv); }
  void addRole(std::string_view sv) { _role = sv; }
  void addPort(std::string_view sv) { _ports.push_back(sv); }
  void addIndex(std::string_view sv) { _index = sv; }
  void addAddress(std::string_view sv) { _addresses.push_back(sv); }

  array_type _addresses;
  array_type _lib_names;
  array_type _resource_names;
  array_type _ports;
  bool _error_parsing = false;
  std::string_view _role = "User";
  std::string_view _index = "0";
};

std::vector<std::string> getInput(char** argv, int argc);

[[nodiscard("Discarding address parsing result")]] std::expected<
    std::array<uint8_t, kIpAddrOctetAmount>, ParseResult>
parseAddr(std::string_view ip_addr);

[[nodiscard(
    "Discarding port parsing result")]] std::expected<uint16_t, ParseResult>
parsePort(std::string_view port);

[[nodiscard(
    "Discarding index parsing result")]] std::expected<size_t, ParseResult>
parseIndex(std::string_view index);

[[nodiscard("Discarding cl_args parse_result")]] std::expected<
    CommandLineArgsHolder, ParseResult>
parseClArgs(std::span<std::string> vec);

};  // namespace parsing_protei

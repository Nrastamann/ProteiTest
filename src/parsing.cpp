#include "parsing.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstring>
#include <expected>
#include <format>
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>
#include <span>
#include <string_view>

#include "custom_types.hpp"
#include "ip_addr.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"

static constexpr size_t kHexBase{16};

namespace parsing {
static std::expected<network_addr::IpAddr, ParseResult> parseAddr(
    std::array<std::string_view, network_addr::kIpAddrOctetAmount + 1>& ip_addr, bool has_hex)
{
  logging::SingleThreadPresets::functionCall();

  auto parser = [](auto& number, auto* it, bool has_hex) {
    std::from_chars_result err_res =
        has_hex ? std::from_chars(it->begin(), it->end(), number, kHexBase)
                : std::from_chars(it->begin(), it->end(), number);

    if (err_res.ec != std::errc() || err_res.ptr != it->end()) {
      logging::SingleThreadPresets::userInputError(*it, *err_res.ptr);
      return false;
    }
    return true;
  };
  network_addr::IpAddr result_addr{};

  const auto* it = std::prev(ip_addr.begin(), 1);

  for (auto& octet : result_addr._addr) {
    std::advance(it, 1);
    if (!parser(octet, it, has_hex)) {
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }
  }

  if (!parser(result_addr._port, std::next(it, 1), has_hex)) {
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return result_addr;
}

static std::expected<size_t, ParseResult> parseIndex(std::string_view index)
{
  logging::SingleThreadPresets::functionCall();

  size_t index_number{};
  auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);
  if (ec != std::errc() || ptr != index.end() || index.size() == 0) {
    logging::SingleThreadPresets::userInputError(index, *ptr);
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return index_number;
}

static std::expected<bool, ParseResult> composeAddr(
    std::array<std::string_view, network_addr::kIpAddrOctetAmount + 1>& result_vector,
    std::string_view ip_addr)
{
  logging::SingleThreadPresets::functionCall();

  bool has_hex = ip_addr.find_first_of("abcdef") != std::string::npos;
  std::string_view digits = has_hex ? "0123456789abcdef" : "0123456789";
  size_t substr_begin = 0;

  for (auto& str : result_vector) {

    size_t end = ip_addr.find_first_not_of(digits, substr_begin);

    if (substr_begin == std::string_view::npos) {
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }

    str = ip_addr.substr(substr_begin,
                         end != std::string_view::npos ? end - substr_begin : ip_addr.size());

    ip_addr.remove_prefix(ip_addr.size() < end ? ip_addr.size() : end);
    substr_begin = ip_addr.find_first_of(digits);
  }

  return has_hex;
}

static std::string composeIndex(std::string_view index_str)
{
  logging::SingleThreadPresets::functionCall();

  std::string_view digits = "0123456789";
  std::string result;
  size_t substr_begin = 0;

  for (; substr_begin != std::string_view::npos;) {
    size_t end = index_str.find_first_not_of(digits, substr_begin);

    result += index_str.substr(
        substr_begin, end != std::string_view::npos ? end - substr_begin : index_str.size());

    index_str.remove_prefix(index_str.size() < end ? index_str.size() : end);
    substr_begin = index_str.find_first_of(digits);
  }

  return result;
}

std::expected<custom_types::PolymorphicVectorQuad, ParseResult> parseStringVector(
    nlohmann::json& json)
{
  logging::SingleThreadPresets::functionCall();
  auto it = json.find("TypeHash");

  if (it == json.end()) {
    logging::SingleThreadPresets::defaultError(
        std::format("Didn't get type from json {}", json.dump()));
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }
  size_t hash = *it;

  it = json.find("Vector");
  if (it == json.end()) {
    logging::SingleThreadPresets::defaultError(
        std::format("Didn't get vector from json {}", json.dump()));
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  std::string vector_unparsed = *it;

  custom_types::PolymorphicVectorQuad vector;

  const auto& default_value = custom_types::getDefaultValues().at(hash);

  std::ranges::fill(vector, default_value);

  //NOLINTNEXTLINE
  std::stringstream streambuf(vector_unparsed.data());

  std::string input_string;

  for (auto& element : vector) {
    streambuf >> input_string;
    auto result = menu_functions::emplaceInVector(element, input_string,
                                                  std::hash<std::string_view>{}(input_string));

    if (result.ec != std::errc()) {
      logging::SingleThreadPresets::defaultError(
          std::format("Couldn't parse element {} of json {}", input_string, json.dump()));
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }
  }
  return vector;
}

bool ArgHolder::pushAddr(std::string token)
{
  logging::SingleThreadPresets::functionCall();
  std::array<std::string_view, network_addr::kIpAddrOctetAmount + 1> ip_addr_to_parse;

  auto compose_result = composeAddr(ip_addr_to_parse, token);

  if (!compose_result.has_value()) {
    logging::SingleThreadPresets::defaultError(
        std::format("Couldn't parse {}, empty octets or non-existing port", token));

    return false;
  }

  auto parsed_addr = parseAddr(ip_addr_to_parse, compose_result.value());

  if (!parsed_addr.has_value()) {
    logging::SingleThreadPresets::defaultError(
        std::format("Couldn't collect address, not enough numbers - {}", token));
    return false;
  }

  _addresses.push_back(parsed_addr.value());
  return true;
}

bool ArgHolder::pushIndex(std::string token)
{
  logging::SingleThreadPresets::functionCall();

  auto parsed_addr = parseIndex(composeIndex(std::move(token)));

  if (!parsed_addr.has_value()) {
    logging::SingleThreadPresets::defaultError(
        std::format("Couldn't collect index - {}", token));
    return false;
  }

  _index = parsed_addr.value();
  return true;
}

static std::string_view flagInside(std::string& args, std::string_view str_to_break, size_t pos)
{
  logging::SingleThreadPresets::functionCall();
  args += str_to_break.substr(0, pos);
  return pos + 2 < str_to_break.size() ? str_to_break.substr(pos + 2, str_to_break.size())
                                       : std::string_view();
}

static std::pair<std::span<char>, std::string_view> checkForFlag(std::string& token,
                                                                 std::string_view delimeter,
                                                                 char* argument)
{
  logging::SingleThreadPresets::functionCall();
  std::span<char> argument_span;
  std::string_view argument_wrapped{argument};
  size_t pos = argument_wrapped.find('-');
  bool parsing_argument = pos == std::string_view::npos;

  if (parsing_argument) {
    token += argument_wrapped;
    token += delimeter;
    return {};
  }

  argument_span = std::span(argument, pos + 2);
  argument_span = argument_span.subspan(pos);
  std::string_view suffix_value = flagInside(token, argument_wrapped, pos);

  return {argument_span, suffix_value};
}

inline static std::expected<bool, ParseResult> checkFlags(ArgHolder::argsMap& flags_map,
                                                          size_t hash, char* argument)
{
  if (!flags_map.contains(hash)) {
    logging::SingleThreadPresets::defaultError(
        std::format("Wrong flag in token - {}", argument));
    return std::unexpected(ParseResult::WRONG_FLAG);
  }

  if (hash == hashed::kHelp) {
    return std::unexpected(ParseResult::HELP);
  }
  return true;
}

namespace {
struct ParsingUtilities {
  std::string _token;
  std::span<char> _argument_span;
  std::string_view _delimeter = "||";
  size_t _hash;
  int _argc_initial;
};
}  // namespace

std::expected<ArgHolder, ParseResult> parseArguments(int argc, char** argv,
                                                     ArgHolder::argsMap& flags_map)
{
  logging::SingleThreadPresets::functionCall();

  ParsingUtilities storage;

  auto argv_wrapped = std::span(argv, static_cast<size_t>(argc));
  argv_wrapped = argv_wrapped.subspan(1);
  argc--;
  storage._argc_initial = argc;
  ArgHolder argument_holder{};
  std::string_view argument_wrapped{};  //for error printing
  std::string token{};

  for (auto* argument : argv_wrapped) {
    argument_wrapped = {argument};

    std::pair<std::span<char>, std::string_view> possible_flag =
        checkForFlag(token, storage._delimeter, argument);

    if (possible_flag.first.empty()) {
      continue;
    }

    storage._argument_span = possible_flag.first;

    if (storage._argc_initial != argc &&
        !argument_holder.setArgument(storage._hash, token, flags_map)) {
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }

    std::ranges::transform(storage._argument_span, storage._argument_span.begin(), ::tolower);

    storage._hash = std::hash<std::string_view>{}(
        std::string_view{storage._argument_span.data(), storage._argument_span.size()});

    storage._delimeter = isNumericFlag(storage._hash) ? "||" : "";

    if (possible_flag.second.size() != 0) {
      token += possible_flag.second;
      token += storage._delimeter;
    }

    if (auto possible_error = checkFlags(flags_map, storage._hash, argument);
        !possible_error.has_value()) {
      return std::unexpected(possible_error.error());
    }

    argc--;
  }

  if (token.size() != 0) {
    if (!argument_holder.setArgument(storage._hash, token, flags_map)) {
      logging::SingleThreadPresets::defaultError(std::format(
          "Couldn't parse token \"{}\" with flag {}", argument_wrapped, storage._hash));
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }
    //fix to parse -a127.0.0.1:5000
    storage._argc_initial++;
  }

  if (storage._argc_initial == 1) {
    logging::SingleThreadPresets::defaultError(
        std::format("Last unpaired flag - {}", storage._argument_span));
    return std::unexpected(ParseResult::NO_ARGUMENT);
  }

  return argument_holder;
}
};  // namespace parsing
// namespace parsing

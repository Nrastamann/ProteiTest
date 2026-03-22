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

#include "ip_addr.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"
#include "settings.hpp"
static constexpr size_t kHexBase{16};

namespace parsing {
static std::expected<network_addr::IpAddr, ParseResult> parseAddr(
    std::array<std::string_view, network_addr::kIpAddrOctetAmount + 1> ip_addr, bool has_hex)
{
  logging::SingleThreadPresets::functionCall();

  std::array<uint8_t, network_addr::kIpAddrOctetAmount + 1> addr_with_port{0};
  const auto* it = std::prev(ip_addr.begin(), 1);

  for (auto& octet : addr_with_port) {
    std::advance(it, 1);
    if (it->size() == 0) {
      logging::SingleThreadPresets::defaultError(
          std::format("Couldn't parse octet {}", static_cast<size_t>(ip_addr.begin() - it)));
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }
    std::from_chars_result err_res =
        has_hex ? std::from_chars(it->begin(), it->end(), octet, kHexBase)
                : std::from_chars(it->begin(), it->end(), octet);

    if (err_res.ec != std::errc() || err_res.ptr != it->end()) {
      logging::SingleThreadPresets::userInputError(*it, *err_res.ptr);
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }
  }

  network_addr::IpAddr result_addr{};

  std::ranges::copy(result_addr._addr, addr_with_port.begin());

  result_addr._port = addr_with_port[network_addr::kIpAddrOctetAmount];
  return result_addr;
}

static std::expected<size_t, ParseResult> parseIndex(std::string_view index)
{
  logging::SingleThreadPresets::functionCall();

  size_t index_number{};
  auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);
  if (ec != std::errc() || ptr != index.end()) {
    logging::SingleThreadPresets::userInputError(index, *ptr);
    return std::unexpected(ParseResult::SV_PARSING_ERR);
  }

  return index_number;
}

static bool composeAddr(
    std::array<std::string_view, network_addr::kIpAddrOctetAmount + 1>& result_vector,
    std::string_view ip_addr)
{
  logging::SingleThreadPresets::functionCall();

  bool has_hex = ip_addr.find_first_of("abcdef") != std::string::npos;
  std::string_view digits = has_hex ? "0123456789abcdef" : "0123456789";

  size_t substr_begin = 0;

  for (auto& str : result_vector) {
    size_t end = ip_addr.find_first_not_of(digits, substr_begin);

    str = ip_addr.substr(substr_begin, end - substr_begin);

    ip_addr.remove_prefix(ip_addr.size() < end ? ip_addr.size() - 1 : end);
    substr_begin = ip_addr.find_first_of(digits);
  }

  return has_hex;
}

custom_types::PolymorphicVectorQuad parseStringVector(nlohmann::json& json)
{
  logging::SingleThreadPresets::functionCall();
  size_t hash = json["TypeHash"];
  std::string vector_unparsed = json["Vector"];

  custom_types::PolymorphicVectorQuad vector;

  const auto& default_value =
      custom_types::getDefaultValues().at(custom_types::getHashToTypeInfo().at(hash).first);

  std::ranges::fill(vector, default_value);

  //NOLINTNEXTLINE
  std::stringstream streambuf(vector_unparsed.data());

  std::string input_string;

  for (auto& element : vector) {
    streambuf >> input_string;
    menu_functions::emplaceInVector(element, input_string,
                                    std::hash<std::string_view>{}(input_string));
  }
  return vector;
}

static std::string_view flagInside(std::string& args, std::string_view str_to_break, size_t pos)
{
  logging::SingleThreadPresets::functionCall();
  args += str_to_break.substr(0, pos);
  return str_to_break.substr(pos + 2);
}

bool ArgHolder::pushAddr(std::string&& token)
{
  logging::SingleThreadPresets::functionCall();
  std::array<std::string_view, network_addr::kIpAddrOctetAmount + 1> ip_addr_to_parse;
  auto parsed_addr =
      parseAddr(ip_addr_to_parse, composeAddr(ip_addr_to_parse, std::move(token)));
  if (!parsed_addr.has_value()) {
    return false;
  }

  _addresses.push_back(parsed_addr.value());
  return true;
}

std::expected<ArgHolder, ParseResult> parseArguments(int argc, char** argv)
{
  logging::SingleThreadPresets::functionCall();
  auto argv_wrapped = std::span(argv, argc);

  ArgHolder argument_holder{};

  std::string token{};
  size_t hash = 0;
  std::string_view suffix_value;

  auto to_span = [](char* element) {
    return std::span(element, strlen(element));
  };

  bool is_next_arg = false;
  std::span<char> argument_span;

  for (auto* argument : argv_wrapped) {
    if (hash == hashed::kHelp) {
      return std::unexpected(ParseResult::HELP);
    }

    if (is_next_arg) {
      std::string_view argument_wrapped = {argument};

      size_t pos = argument_wrapped.find('-');
      is_next_arg = pos == std::string_view::npos;

      if (is_next_arg) {
        token += argument_wrapped;
        continue;
      }

      argument_span = std::span(argument, pos + 2);
      argument_span = argument_span.subspan(pos);

      flagInside(token, argument_wrapped, pos);
    }
    else {
      argument_span = to_span(argument);
    }

    argument_holder.setArgument(hash, token);
    token.resize(0);

    if (suffix_value.size() != 0) {
      token += suffix_value;
      suffix_value.remove_suffix(suffix_value.size());
    }

    std::ranges::transform(argument_span, argument_span.begin(), ::tolower);

    hash = std::hash<std::string_view>{}(argument);
    is_next_arg = true;
  }

  bool is_valid_argument = argument_holder.setArgument(hash, token);

  if (!is_valid_argument) {
    return std::unexpected(ParseResult::WRONG_FLAG);
  }

  if (std::hash<std::string_view>{}(token) == hash) {
    logging::SingleThreadPresets::defaultError(
        std::format("Last unpaired flag - {}", argv_wrapped.last(1)));

    return std::unexpected(ParseResult::NO_ARGUMENT);
  }

  return argument_holder;
}

std::expected<AppSettings, bool> createSettings(std::vector<std::string> wrapped_input,
                                                std::string_view helpText)
{
  using log_pr = logging::SingleThreadPresets;
  log_pr::functionCall();

  auto argv_split{1};  // = parsing::(wrapped_input);
/*
  switch (argv_split.error_or(parsing::ParseResult::NO_ERR)) {
    case parsing::ParseResult::WRONG_FLAG:
      log_pr::defaultError("Wrong flag passed");
      return std::unexpected(false);

    case parsing::ParseResult::NO_ARGUMENT:
      log_pr::defaultError("Flag with argument passed without one");
      return std::unexpected(false);
    case parsing::ParseResult::HELP:
      std::cout << helpText;
      return std::unexpected(true);
    default:
      break;
  }

  log_pr::createObject<AppSettings>();
  AppSettings command_line_options{argv_split->getPorts(), argv_split->getLibs(),
                                   argv_split->getAddresses(), argv_split->getRole(),
                                   argv_split->getIndex()};

  if (command_line_options.cgetShouldClose() || argv_split->parsingStatus()) {
    log_pr::defaultError("Couldn't get resource or parse cl args");
    return std::unexpected(false);
  }

  return command_line_options;
*/}

};  // namespace parsing

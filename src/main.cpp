#include <algorithm>
#include <array>
#include <expected>
#include <functional>
#include <ranges>
#include <string>
#include <string_view>

#include "config.h"
#include "data_pool.hpp"
#include "display.hpp"
#include "io_manager.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"
#include "static_containers.hpp"

namespace rv = std::ranges::views;

int main(int argc, char* argv[])
{
  auto& in = protei_io::io().cin();
  Logger::loggerInit();

  auto argv_split = parsing_protei::parseClArgs(argv, argc);

  switch (argv_split.error_or(parsing_protei::ParseResult::NO_ERR)) {
    case parsing_protei::ParseResult::WRONG_FLAG:
      logger_presets::defaultError("Wrong flag passed");
      return 1;
    case parsing_protei::ParseResult::NO_ARGUMENT:
      logger_presets::defaultError("Flag with argument passed without one");
      return 1;
    default:
  }

  using addr_parse_result =
      std::expected<std::array<uint8_t, kIpAddrOctetAmount>,
                    parsing_protei::ParseResult>;
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> ip_arr;

  auto addresses_view = argv_split->_addresses |
                        rv::transform(&parsing_protei::parseAddr) |
                        rv::take_while(&addr_parse_result::has_value) |
                        rv::transform([](const auto& a) { return a.value(); });

  for (const auto& i : addresses_view) {
    ip_arr.push_back(i);
  }

  if (ip_arr.size() != argv_split->_addresses.size()) {
    return 1;
  }

  using port_parse_result = std::expected<size_t, parsing_protei::ParseResult>;
  std::vector<size_t> ports_arr;

  auto ports_view = argv_split->_ports |
                    rv::transform(&parsing_protei::parsePort) |
                    rv::take_while(&port_parse_result::has_value) |
                    rv::transform([](const auto& a) { return a.value(); });

  for (const auto& i : ports_view) {
    ports_arr.push_back(i);
  }

  if (ports_arr.size() != argv_split->_ports.size()) {
    return 1;
  }

  std::expected<size_t, parsing_protei::ParseResult> index =
      parsing_protei::parseIndex(argv_split->_index);

  if (!index.has_value()) {
    logger_presets::defaultError("Couldn't parse the index");
    return 1;
  }

  logger_presets::createObject<AppSettings>();
  AppSettings command_line_options{ports_arr, argv_split->_lib_names, ip_arr,
                                   argv_split->_role, index.value()};

  if (command_line_options.cgetShouldClose()) {
    return 1;
  }

  ui_protei::displayMenu();

  //if i have reference to static object, as I know, there won't be any additional
  //calls to check if static object is initialized
  const std::unordered_map<size_t, static_containers::MenuOptions>&
      menu_options = static_containers::getMenuOptions();

  Menu menu;

  logger_presets::createObject<DataPool>();
  DataPool data_pool;

  logger_presets::createObject<FunctionArgs>();
  FunctionArgs arguments{command_line_options, data_pool};

  while (!command_line_options.cgetShouldClose()) {
    std::string text_option;

    Logger::writeToLogNCl<config::LogVerbosity::Debug>("Your command: ");

    in >> text_option;
    Logger::writeToLog<config::LogVerbosity::Debug>(text_option);
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = menu_options.contains(input_hash);

    static_containers::MenuOptions picked_option =
        is_correct_option ? menu_options.at(input_hash)
                          : static_containers::MenuOptions::WrongOption;

    menu.callFunction(picked_option, 0, 0, arguments);
  }
  ui_protei::clearScreen();
}

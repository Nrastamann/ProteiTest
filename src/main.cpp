#include <algorithm>
#include <array>
#include <expected>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

#include "config.h"
#include "data_pool.hpp"
#include "display.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"
#include "static_containers.hpp"

int main(int argc, char* argv[])
{
  Logger::loggerInit();
  DataPool data_pool;

  auto argv_split = parsing_protei::parseClArgs(argv, argc);

  switch (argv_split.error_or(parsing_protei::ParseResult::NO_ERR)) {
    case parsing_protei::ParseResult::WRONG_FLAG:
      Logger::writeToLogNCl<config::LogVerbosity::Error>("Wrong flag passed");
      return 1;
      break;
    case parsing_protei::ParseResult::NO_ARGUMENT:
      Logger::writeToLogNCl<config::LogVerbosity::Error>(
          "Flag with argument passed without one");
      return 1;
    default:
      Logger::writeToLog<config::LogVerbosity::Debug>(
          "Argv moved to arguments holder successfully");

      break;
  }

  std::vector<std::expected<std::array<uint8_t, kIpAddrOctetAmount>,
                            parsing_protei::ParseResult>>
      ip_addresses(argv_split->_addresses.size());

  std::ranges::transform(
      argv_split->_addresses, ip_addresses.begin(),
      [](std::string_view sv) { return parsing_protei::parseAddr(sv); });

  if (std::ranges::any_of(
          ip_addresses,
          [](const std::expected<std::array<uint8_t, kIpAddrOctetAmount>,
                                 parsing_protei::ParseResult>& res) {
            return !res.has_value();
          })) {

    Logger::writeToLogNCl<config::LogVerbosity::Error>(
        "Couldn't parse one of the ip_addresses");
    return 1;
  }
  std::vector<std::array<uint8_t, kIpAddrOctetAmount>> ip_arr(
      ip_addresses.size());

  std::ranges::transform(
      ip_addresses, ip_arr.begin(),
      [](std::expected<std::array<uint8_t, kIpAddrOctetAmount>,
                       parsing_protei::ParseResult>
             a) { return a.value(); });

  std::vector<std::expected<size_t, parsing_protei::ParseResult>> ports(
      argv_split->_ports.size());

  std::ranges::transform(
      argv_split->_ports, ports.begin(),
      [](std::string_view sv) { return parsing_protei::parsePort(sv); });

  if (std::ranges::any_of(
          ports,
          [](const std::expected<size_t, parsing_protei::ParseResult>& res) {
            return !res.has_value();
          })) {

    Logger::writeToLogNCl<config::LogVerbosity::Error>(
        "Couldn't parse one of the ports");
    return 1;
  }
  std::vector<size_t> ports_arr(ports.size());

  std::ranges::transform(
      ports, ports_arr.begin(),
      [](std::expected<size_t, parsing_protei::ParseResult> a) {
        return a.value();
      });

  std::expected<size_t, parsing_protei::ParseResult> index =
      parsing_protei::parseIndex(argv_split->_index);

  if (!index.has_value()) {
    Logger::writeToLogNCl<config::LogVerbosity::Error>("Couldn't parse index");
    return 1;
  }

  Logger::writeToLog<config::LogVerbosity::Debug>(
      "Starting AppSettings construction");
  AppSettings command_line_options{ports_arr, argv_split->_lib_names, ip_arr,
                                   argv_split->_role, index.value()};

  ui_protei::displayMenu();

  //if i have reference to static object, as I know, there won't be any additional
  //calls to check if static object is initialized
  const std::unordered_map<size_t, static_containers::MenuOptions>&
      menu_options = static_containers::getMenuOptions();

  Menu menu;

  FunctionArgs arguments{command_line_options, data_pool};

  Logger::writeToLog<config::LogVerbosity::Debug>("Starting main-loop");

  while (!command_line_options.cgetShouldClose()) {
    std::string text_option;

    Logger::writeToLog<config::LogVerbosity::Debug>("Starting main-loop");

    Logger::writeToLogNCl<config::LogVerbosity::Trace>("Your command: ");

    std::cin >> text_option;
    Logger::writeToLog<config::LogVerbosity::Trace>(text_option);
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = menu_options.contains(input_hash);

    static_containers::MenuOptions picked_option =
        is_correct_option ? menu_options.at(input_hash)
                          : static_containers::MenuOptions::WrongOption;

    Logger::writeToLog<config::LogVerbosity::Debug>("Call function");
    menu.callFunction(picked_option, 0, 0, arguments);
  }
  ui_protei::clearScreen();
}

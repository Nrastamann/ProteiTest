#include <algorithm>
#include <expected>
#include <functional>
#include <string>
#include <string_view>

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
  std::vector<std::string> wrapped_input = getInput(argv, argc);

  auto argv_split = parsing_protei::parseClArgs(wrapped_input);

  switch (argv_split.error_or(parsing_protei::ParseResult::NO_ERR)) {
    case parsing_protei::ParseResult::WRONG_FLAG:
      logger_presets::defaultError("Wrong flag passed");
      return 1;
    case parsing_protei::ParseResult::NO_ARGUMENT:
      logger_presets::defaultError("Flag with argument passed without one");
      return 1;
    default:
  }

  logger_presets::createObject<AppSettings>();
  AppSettings command_line_options{
      argv_split->getPorts(), argv_split->getLibs(), argv_split->getAddresses(),
      argv_split->getRole(), argv_split->getIndex()};

  if (command_line_options.cgetShouldClose() || argv_split->parsingStatus()) {
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

    std::cin >> text_option;
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

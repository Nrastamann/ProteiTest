#include <expected>
#include <string>
#include <string_view>

#include "data_pool.hpp"
#include "display.hpp"

#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"

int main(int argc, char* argv[])
{
  Logger::loggerInit();
  std::vector<std::string> wrapped_input = parsing_protei::getInput(argv, argc);

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
    logger_presets::defaultError("Flag with argument passed without one");
    return 1;
  }

  ui_protei::displayMenu();

  Menu menu;

  logger_presets::createObject<DataPool>();
  DataPool data_pool;

  logger_presets::createObject<FunctionArgs>();
  FunctionArgs arguments{command_line_options, data_pool};

  while (!command_line_options.cgetShouldClose()) {
    menu.menuTask(0, 0, arguments);
  }
  ui_protei::clearScreen();
}

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
  namespace log_pr = logging::logger_presets;

  logging::Logger::loggerInit();
  std::vector<std::string> wrapped_input = parsing::getInput(argv, argc);

  auto argv_split = parsing::parseClArgs(wrapped_input);

  switch (argv_split.error_or(parsing::ParseResult::NO_ERR)) {
    case parsing::ParseResult::WRONG_FLAG:
      log_pr::defaultError("Wrong flag passed");
      return 1;
    case parsing::ParseResult::NO_ARGUMENT:
      log_pr::defaultError("Flag with argument passed without one");
      return 1;
    default:
      break;
  }

  log_pr::createObject<AppSettings>();
  AppSettings command_line_options{
      argv_split->getPorts(), argv_split->getLibs(), argv_split->getAddresses(),
      argv_split->getRole(), argv_split->getIndex()};

  if (command_line_options.cgetShouldClose() || argv_split->parsingStatus()) {
    log_pr::defaultError("Couldn't get resource or parse cl args");
    return 1;
  }

  display::displayMenu();

  Menu menu;

  log_pr::createObject<data_storage::DataPool>();
  data_storage::DataPool data_pool;

  log_pr::createObject<FunctionArgs>();
  FunctionArgs arguments{command_line_options, data_pool};

  while (!command_line_options.cgetShouldClose()) {
    menu.menuTask(0, 0, arguments);
  }
  display::clearScreen();
}

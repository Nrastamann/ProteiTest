#include <expected>
#include <string>
#include <string_view>

#include "data_pool.hpp"
#include "display.hpp"

#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"

static std::expected<AppSettings, bool> parsingArguments(std::vector<std::string> wrapped_input)
{
  namespace log_pr = logging::logger_presets;
  log_pr::functionCall();

  auto argv_split = parsing::parseClArgs(wrapped_input);

  switch (argv_split.error_or(parsing::ParseResult::NO_ERR)) {
    case parsing::ParseResult::WRONG_FLAG:
      log_pr::defaultError("Wrong flag passed");
      return std::unexpected(false);

    case parsing::ParseResult::NO_ARGUMENT:
      log_pr::defaultError("Flag with argument passed without one");
      return std::unexpected(false);

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
}

int main(int argc, char* argv[])
{
  namespace log_pr = logging::logger_presets;

  logging::Logger::loggerInit();
  auto command_line_options = parsingArguments(parsing::getInput(argv, argc));

  if (!command_line_options.has_value()) {
    return 1;
  }

  display::displayMenu();

  Menu menu;

  log_pr::createObject<data_storage::DataPool>();
  data_storage::DataPool data_pool;

  log_pr::createObject<FunctionArgs>();
  FunctionArgs arguments{command_line_options.value(), data_pool};

  while (!command_line_options->cgetShouldClose()) {
    menu.menuTask(0, 0, arguments);
  }
  display::clearScreen();
}

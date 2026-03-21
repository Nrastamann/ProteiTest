#include <expected>
#include <string>
#include <string_view>

#include "config.hpp"
#include "data_pool.hpp"
#include "display.hpp"

#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"
static constexpr std::string_view kHelpText =
    "Usage: proteip: [-a server_address [port]] [-p server_port] [-i index]\
[-r role] [-l - lib_name] [-h help]\
\
-a - IP Адрес сервера в следующих форматах:\
- Hex - ff.0f.00.01\
- Десятичный формат - 127.0.0.1\
- Через пробелы - ff 0f 00 01\
- Возможно указание порта - 127 0.0 1 5000        \
- Любые символы кроме шестнадцетиричных цифр и чисел будут пропущены 127iuuuy0uijk0l0k1 5000\
-p - Порт сервера\
-i - Индекс пользователя, выводится через опцию settings(Не используется)\
-r - Роль пользователя, выводится через опцию settings(Не используется)\
-l - Путь к файлу, используется в ResourceTest, проверяет на существование(Не используется)\
-h - справка\
\
Пример использования:\
./proteip -a 127.0.0.1 -p 8888 -a 7f.0.0.1 5000 -a 127.0.0.1:4222 -r RoleName -i 11 -l src/\
\
В меню пользователи Доделать хелпу и добавить в сервере, ридми, отключение/включение логов по флагу, перепривязать \
    парсинг аргументов в сервере такой же как и в клиенте ";

static std::expected<AppSettings, bool> parsingArguments(std::vector<std::string> wrapped_input)
{
  using log_pr = logging::SingleThreadPresets;
  log_pr::functionCall();

  auto argv_split = parsing::parseClArgs(wrapped_input);

  switch (argv_split.error_or(parsing::ParseResult::NO_ERR)) {
    case parsing::ParseResult::WRONG_FLAG:
      log_pr::defaultError("Wrong flag passed");
      return std::unexpected(false);

    case parsing::ParseResult::NO_ARGUMENT:
      log_pr::defaultError("Flag with argument passed without one");
      return std::unexpected(false);
    case parsing::ParseResult::HELP:
      std::cout << kHelpText;
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
}

int main(int argc, char* argv[])
{
  using log_pr = logging::SingleThreadPresets;

  logging::SingleThreadLogger::loggerInit();
  auto command_line_options = parsingArguments(parsing::getInput(argv, argc));

  if (!command_line_options.has_value()) {
    logging::SingleThreadLogger::writeToLog<config::LogVerbosity::Error>("why");
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

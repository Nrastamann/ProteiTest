#include <expected>
#include <string_view>

#include "config.hpp"
#include "data_pool.hpp"
#include "display.hpp"

#include "logger.hpp"
#include "menu.hpp"
#include "parsing.hpp"
#include "settings.hpp"

static constexpr std::string_view kHelpText =
    "Usage: proteip: [-a server_address port] [-i index] [-r role] [-l - lib_name] [-h help]\n\
\n\
\
-a - IP Адрес сервера в следующих форматах:\n\
        - Hex - ff.0f.00.01:5000\n\
        - Десятичный формат - 127.0.0.1:5000\n\
        - Через пробелы - ff 0f 00 01:5000\n\
        - Возможно указание порта - 127 0.0 1 5000\n\
        - Любые символы кроме шестнадцетиричных и десятичных цифр будут пропущены\
        127iuuuy0uijk0l1k_tsts5000 (будет получен адрес 127.0.0.1:5000)\n\
-i - Индекс пользователя, выводится через опцию settings(Не используется), \
при вводе игнорируются любые символы кроме десятичных цифр\n\
-r - Роль пользователя, выводится через опцию settings(Не используется)\n\
-l - Путь к файлу, используется в ResourceTest, проверяет на существование(Не используется)\n\
-h - справка\n\
\n\
Пример использования:\n\
./proteip -a 127.0.0.1 som_txt_to_tst_tht_works_8888 -a 7f.0.0.1 5000 -a 127.0.0.1:4222 -r R o l e N a m e -i 1 1 -l s r c/\n\
На вход программа получит следующие аргументы:\n\
        -IP-адрес 127.0.0.1 с портом 8888\n\
        -IP-адрес 7f.0.0.1 с портом 5000\n\
        -IP-адрес 127.0.0.1 с портом 4222\n\
        -Роль RoleName\n\
        -Индекс 11\n\
        -Путь для проверки src\n\n\
\
После пользователь увидит меню, в котором описаны опции, которые можно указывать\n\
текстом для использования, у каждой опции меню есть собственое маленькое описание функционала\n\n\
Особенности клиента:\n\
        - Поддержка различных типов введенных аргументов, IP-Адрес не распарсится, если\n\
        в тексте будут обнаружены лишние октеты(в виде шестнадцетеричных цифр или же число введенных\n\
        \nоктетов не равно 4 + порт), если внутри одного токена будет больше одного флага -\n\
        ./build/debug-san-clang/proteip -a127.0.0.1:5000-a127.0.0.1:5001 - не\n\
        распарсится и еще в нескольких случаях\n\
        В остальных случаях IP-Адрес будет распарсен в соответствии с\n\
        вводом, включая подобный ввод:\n\
        ./build/debug-san-clang/proteip ./build/debug-san-clang/proteip\n\
        -a127.0.0.1:5000 -a 127FKOJ.0.OJP0.OJ1:JHL5001\n\
        Будет получены адреса 127.0.0.1 с портами 5000 5001 и 5002\n\
        - Разделение логгера на две версии - многопоточную и однопоточную, где\n\
        при необходимости можно получить разное поведение (вывод номера потока)\n\
        - При необходимости, можно передать несколько адресов, тогда на каждый сервер будет\n\
        отправляться по экземпляру вектора и каждый вернет ответ в dataPool, можно\n\
        реализовать различное поведение на серверах и получать разные данные\n";

int main(int argc, char* argv[])
{
  using log_pr = logging::SingleThreadPresets;

  logging::SingleThreadLogger::loggerInit();
  std::expected<parsing::ArgHolder, parsing::ParseResult> parsed_arguments =
      parsing::parseArguments(argc, argv, parsing::getArgSetterMain());

  switch (parsed_arguments.error_or(parsing::ParseResult::NO_ERR)) {
    case parsing::ParseResult::HELP:
      std::cout << kHelpText;
      return 0;
    case parsing::ParseResult::WRONG_FLAG:
      logging::SingleThreadLogger::writeToLogNCl<config::LogVerbosity::Error>(
          "Non-existing flag");
      return 1;
    case parsing::ParseResult::NO_ARGUMENT:
      logging::SingleThreadLogger::writeToLogNCl<config::LogVerbosity::Error>("Unpaired flag");
      return 1;

    case parsing::ParseResult::SV_PARSING_ERR:
      logging::SingleThreadLogger::writeToLogNCl<config::LogVerbosity::Error>(
          "Couldn't parse arguments");
      return 1;
    default:
      break;
  }

  AppSettings settings(parsed_arguments.value());

  if (settings.cgetShouldClose()) {
    return 1;
  }

  display::displayMenu();

  Menu menu;

  log_pr::createObject<data_storage::DataPool>();
  data_storage::DataPool data_pool;

  log_pr::createObject<FunctionArgs>();
  FunctionArgs arguments{settings, data_pool};

  while (!settings.cgetShouldClose()) {
    menu.menuTask(0, 0, arguments);
  }
  display::clearScreen();
}

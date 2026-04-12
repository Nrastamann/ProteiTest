#include "server.hpp"
#include <sys/socket.h>
#include "config.hpp"
#include "logger.hpp"
#include "mncs_basestation.hpp"
#include "mncs_listener.hpp"
#include "mncs_mme.hpp"
#include "mncs_ueconnection.hpp"
#include "parsing.hpp"
#include "xlr.hpp"
static constexpr std::string_view kHelpText =
    "Usage: proteip.server -p port [-h help] [-v verbosity]\n\
\n\
\
-p - порт на котором запущен сервер\n\
-h - справка\n\
-v - уровень логгирования (Error, Warning, Info, Debug, Trace)\n\
\n\
Пример использования:\n\
./proteip.server -p 4444\n\
На вход программа получит следующие аргументы:\n\
       -Сервер запустится на 4444 порту\n\n\
\
После этого сервер начинает бесконечно долго принимать запросы от клиентов, записывая сообщения в лог\n\
\n\n\
Особенности сервера:\n\
        - Многопоточная работа для 4 клиентов основана на thread-пуле\n\
        - Сервер выполняет следующие операции над данными:\n\
                -Целочисленные данные, в вектор записываются 4 новых значения по принципу -\n\
                {vec[0]+vec[0], vec[0]-vec[1], vec[0] * vec[2], vec[0] / vec[3]} с учетом деления на 0\n\
                -Числа с плавающей точкой, в вектор записываются 4 новых значения по принципу -\n\
                {vec[0]+vec[0], vec[0]-vec[1], vec[0] * vec[2], vec[0] / vec[3]} без учета\n\
                деления на 0(могут быть NaN/Inf)\n\
                -Булевые значения, в вектор записываются 4 новых значения по принципу -\n\
                {vec[0]||vec[0], vec[0]&&vec[1], !vec[0] || !vec[2], !vec[0] && !vec[3]}\n\
                -Строковые данные - к каждой строке применяется функция toupper\n\
                - Разделение логгера на две версии - многопоточную и однопоточную, где\n\
        при необходимости можно получить разное поведение (вывод номера потока)\n\
        - Для теста работы сервера под нагрузкой можно воспользоваться скриптом multithread.sh\n\
        у которого также есть help.\n";

namespace server {
inline static parsing::ArgHolder::argsMap& getArgSetterServer()
{
  static parsing::ArgHolder::argsMap map = {
      {hashed::kPort,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushIMEI(std::move(value));
       }},
      {hashed::kHelp, [](std::string&, parsing::ArgHolder&) { return true; }},
      {hashed::kVerbosity,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.setLog(std::move(value), logging::MultithreadingPolicy{});
       }},
  };

  return map;
}

int serverStart(int argc, char** argv)
{
  logging::MultithreadPresets::functionCall();
  auto result = parsing::parseArguments(argc, argv, getArgSetterServer());

  switch (result.error_or(parsing::ParseResult::NO_ERR)) {
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

  auto port = static_cast<uint16_t>(result->getIMEI());
  if (port == 0) {
    logging::SingleThreadLogger::writeToLogNCl<config::LogVerbosity::Error>(
        "Need port to work propperly\n");

    std::cout << kHelpText;
    return 1;
  }
  std::unordered_map<size_t, std::unique_ptr<mncs::BaseStation>> ptrs;
  mncs::XLR xlr;
  mncs::Listener listener{port, ptrs};
  mncs::MME mme;
  if (!listener.getStatus()) {
    return 0;
  }

  //std::thread worker1(&mncs::MME::run, &mme, std::ref(xlr));
  // worker1.detach();
  listener.startListener();
  return 0;
}

}  // namespace server

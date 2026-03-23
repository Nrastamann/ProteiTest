#include "server.hpp"
#include <sys/socket.h>
#include <algorithm>
#include "config.hpp"
#include "logger.hpp"
#include "parsing.hpp"
#include "thread_pool.hpp"

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
         return holder.pushIndex(std::move(value));
       }},
      {hashed::kHelp, [](std::string&, parsing::ArgHolder&) { return true; }},
      {hashed::kVerbosity,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.setLog(std::move(value), logging::MultithreadingPolicy{});
       }},
  };

  return map;
}

void dataManipulation(std::string& result, custom_types::PolymorphicVectorQuad& vector)
{
  logging::MultithreadPresets::functionCall();

  custom_types::PolymorphicVectorQuad vector_spare = vector;
  size_t index_function_map = 0;
  auto bool_functions = ServerFunctions::getBoolFunc();

  for (auto& element : vector_spare) {
    std::visit(
        custom_types::Visitor{
            [&vector, &index_function_map]<std::floating_point T>(T& val) {
              val = ServerFunctions::fpCalculation<T>(
                  static_cast<CalculationType>(index_function_map++),
                  std::get<T>(*vector.begin()), val);
            },
            [&vector, &index_function_map]<std::integral T>(T& val) {
              val = ServerFunctions::integerCalc<T>(
                  static_cast<CalculationType>(index_function_map++),
                  std::get<T>(*vector.begin()), val);
            },
            [&vector, &index_function_map, &bool_functions](bool& logic) {
              logic = bool_functions.at(index_function_map++)(std::get<bool>(*vector.begin()),
                                                              logic);
            },
            [](std::string& str) { std::ranges::transform(str, str.begin(), ::toupper); }},
        element);

    std::visit(custom_types::Visitor{[&result](const auto& element) {
                 result += std::format("{} ", element);
               }},
               element);
  }
  vector = std::move(vector_spare);
}
void serverTask(int client, BufferPool<kThreadNum>& buffer_pool)
{
  logging::MultithreadPresets::functionCall();
  SocketWrapper client_socket{};
  client_socket._socket = client;

  auto buffer_index = buffer_pool.getBuffer();

  std::array<char, kBufferSize>& string_buffer = buffer_pool._buffers[buffer_index];
  nlohmann::json& json_buffer = buffer_pool._jsons[buffer_index];

  std::ranges::fill(string_buffer, 0);

  auto symbols_read =
      recv(client_socket._socket, string_buffer.begin(), string_buffer.size(), 0);

  if (symbols_read == 0) {
    logging::MultithreadLogger::writeToLog<config::LogVerbosity::Warning>(
        "Client disconnected");
    return;
  }

  if (symbols_read == -1) {
    logging::MultithreadLogger::writeToLogNCl<config::LogVerbosity::Error>("Read Error");
    return;
  }

  json_buffer =
      nlohmann::json::parse(string_buffer.begin(), string_buffer.begin() + symbols_read);

  logging::MultithreadLogger::writeToLogNCl<config::LogVerbosity::Info>(
      std::format("{}", json_buffer.dump(4)));

  custom_types::PolymorphicVectorQuad vector = parsing::parseStringVector(json_buffer);

  std::string result;
  dataManipulation(result, vector);

  json_buffer["Vector"] = result;
  result = json_buffer.dump();

  if (send(client_socket._socket, result.data(), result.length(), 0) == -1) {
    logging::MultithreadLogger::writeToLogNCl<config::LogVerbosity::Error>(
        "Coudn't send answer back");
  }
  logging::MultithreadLogger::writeToLog<config::LogVerbosity::Info>("Thread done");
}

SocketWrapper serverSetup(uint16_t port)
{
  logging::MultithreadPresets::functionCall();

  SocketWrapper server_socket{};

  server_socket._socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket._socket == -1) {
    logging::MultithreadPresets::defaultError("Couldn't init server socket\n");
    return server_socket;
  }

  getSetServerSocket(WriteSocketN{}, server_socket._socket);

  sockaddr_in server_addr{
      .sin_family = AF_INET, .sin_port = htons(port), .sin_addr{}, .sin_zero{}};

  server_addr.sin_addr.s_addr = INADDR_ANY;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(server_socket._socket, reinterpret_cast<sockaddr*>(&server_addr),
           sizeof(server_addr)) == -1) {
    server_socket._socket = -1;
    logging::MultithreadPresets::defaultError("Couldn't init server socket\n");
    return server_socket;
  }

  if (listen(server_socket._socket, kThreadNum) == -1) {
    logging::MultithreadPresets::defaultError("Couldn't init server socket\n");
    server_socket._socket = -1;
    return server_socket;
  }

  return server_socket;
}

int serverStart(int argc, char** argv)
{
  logging::MultithreadPresets::functionCall();
  thread_pool::ThreadPool threads(kThreadNum);

  server::BufferPool<kThreadNum> buffers{};

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

  auto port = static_cast<uint16_t>(result->getIndex());
  if (port == 0) {
    logging::SingleThreadLogger::writeToLogNCl<config::LogVerbosity::Error>(
        "Need port to work propperly\n");

    std::cout << kHelpText;
    return 1;
  }

  SocketWrapper server_socket = serverSetup(port);
  if (server_socket._socket == -1) {
    logging::MultithreadPresets::defaultError("Couldn't create socket");
    return -1;
  }

  // TODO(nrastamann): Need somehow test this inside gtest
  //  setGetServerSocket(SetTag{}, server_socket._socket);

  while (true) {
    int client_socket{};

    client_socket = accept(server_socket._socket, nullptr, nullptr);

    if (client_socket == -1) {
      logging::MultithreadPresets::defaultError("Couldn't init client socket\n");
      return -1;
    }

    threads.addTask(serverTask, client_socket, std::ref(buffers));
  }

  return 0;
}

}  // namespace server

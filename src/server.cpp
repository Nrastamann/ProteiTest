#include "server.hpp"
#include <algorithm>
#include "logger.hpp"
#include "parsing.hpp"
#include "thread_pool.hpp"

namespace server {
void dataManipulation(std::string& result, custom_types::PolymorphicVectorQuad& vector)
{
  logging::logger_presets::functionCall();

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
void serverTask(int client_socket, BufferPool<kThreadNum>& buffer_pool)
{
  logging::logger_presets::functionCall();

  size_t buffer_id{};
  BufferPool<kThreadNum>::BufferPair buffer = buffer_pool.getBuffer(buffer_id);

  std::ranges::fill(buffer.first, 0);

  auto symbols_read = recv(client_socket, buffer.first.data(), buffer.first.size(), 0);

  if (symbols_read == 0) {
    logging::Logger::writeToLog<config::LogVerbosity::Warning>("Client disconnected");
    close(client_socket);
    return;
  }

  if (symbols_read == -1) {
    logging::Logger::writeToLogNCl<config::LogVerbosity::Error>("Read Error");
    return;
  }

  buffer.second = nlohmann::json::parse(buffer.first.begin(), buffer.first.end());

  logging::Logger::writeToLogNCl<config::LogVerbosity::Info>(
      std::format("{}", buffer.second.dump(4)));

  custom_types::PolymorphicVectorQuad vector = parsing::parseStringVector(buffer.second);

  std::string result;
  dataManipulation(result, vector);

  buffer.second["Vector"] = result;
  result = buffer.second.dump();

  if (send(client_socket, result.data(), result.length(), 0) == -1) {
    logging::Logger::writeToLogNCl<config::LogVerbosity::Error>("Coudn't send answer back");
  }
}
std::expected<uint16_t, bool> parsePortServer(std::string_view port)
{
  logging::logger_presets::functionCall();
  logging::logger_presets::functionCall();
  uint16_t port_num = kPortNum;

  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_num);
  if (ec != std::errc() || ptr != port.end()) {
    logging::logger_presets::userInputError(port, *ptr);
    return std::unexpected(true);
  }

  return port_num;
}
SocketWrapper serverSetup(uint16_t port)
{
  logging::logger_presets::functionCall();

  SocketWrapper server_socket{};

  server_socket._socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket._socket == -1) {
    logging::logger_presets::defaultError("Couldn't init server socket\n");
    return server_socket;
  }

  sockaddr_in server_addr{
      .sin_family = AF_INET, .sin_port = htons(port), .sin_addr{}, .sin_zero{}};

  server_addr.sin_addr.s_addr = INADDR_ANY;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(server_socket._socket, reinterpret_cast<sockaddr*>(&server_addr),
           sizeof(server_addr)) == -1) {
    server_socket._socket = -1;
    logging::logger_presets::defaultError("Couldn't init server socket\n");
    return server_socket;
  }

  if (listen(server_socket._socket, kThreadNum) == -1) {
    logging::logger_presets::defaultError("Couldn't init server socket\n");
    server_socket._socket = -1;
    return server_socket;
  }

  return server_socket;
}

int serverStart(int argc, char** argv)
{
  logging::Logger::loggerInit("LogsServer");

  logging::logger_presets::functionCall();

  thread_pool::ThreadPool<kThreadNum> threads{};
  server::BufferPool<kThreadNum> buffers{};

  auto span_argv = std::span(argv, static_cast<size_t>(argc));
  span_argv = span_argv.subspan(1);
  argc--;

  if (argc > 1 || argc == 0) {
    logging::logger_presets::defaultError("Wrong amount of arguments\n");
    return -1;
  }

  std::expected<uint16_t, bool> port = kPortNum;

  if (argc != 0) {
    port = parsePortServer(*span_argv.begin());
  }

  if (!port.has_value()) {
    logging::logger_presets::defaultError("Parse error");
    return -1;
  }
  SocketWrapper server_socket = serverSetup(port.value());
  if (server_socket._socket == -1) {
    logging::logger_presets::defaultError("Couldn't create socket");
    return -1;
  }

  SocketWrapper client_socket{};
  std::atomic<bool>& running = getServerRunning();
  while (running) {
    client_socket._socket = accept(server_socket._socket, nullptr, nullptr);

    if (client_socket._socket == -1) {
      logging::logger_presets::defaultError("Couldn't init client socket\n");
      return -1;
    }

    threads.addTask(serverTask, client_socket._socket, std::ref(buffers));
  }

  return 0;
}

}  // namespace server

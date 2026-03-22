#include "server.hpp"
#include <sys/socket.h>
#include <algorithm>
#include "config.hpp"
#include "logger.hpp"
#include "parsing.hpp"
#include "thread_pool.hpp"

namespace server {
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
std::expected<uint16_t, bool> parsePortServer(std::string_view port)
{
  logging::MultithreadPresets::functionCall();
  logging::MultithreadPresets::functionCall();
  uint16_t port_num = kPortNum;

  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_num);
  if (ec != std::errc() || ptr != port.end()) {
    logging::MultithreadPresets::userInputError(port, *ptr);
    return std::unexpected(true);
  }

  return port_num;
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

  auto span_argv = std::span(argv, static_cast<size_t>(argc));
  span_argv = span_argv.subspan(1);
  argc--;

  if (argc > 1 || argc == 0) {
    logging::MultithreadPresets::defaultError("Wrong amount of arguments\n");
    return -1;
  }

  std::expected<uint16_t, bool> port = kPortNum;

  if (argc != 0) {
    port = parsePortServer(*span_argv.begin());
  }

  if (!port.has_value()) {
    logging::MultithreadPresets::defaultError("Parse error");
    return -1;
  }
  SocketWrapper server_socket = serverSetup(port.value());
  if (server_socket._socket == -1) {
    logging::MultithreadPresets::defaultError("Couldn't create socket");
    return -1;
  }

  setGetServerSocket(SetTag{}, server_socket._socket);

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

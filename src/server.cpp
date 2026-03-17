#include <netinet/in.h>
#include <sys/socket.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <concepts>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "config.hpp"
#include "custom_types.hpp"
#include "logger.hpp"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "parsing.hpp"

constexpr uint16_t kPortNum{5001};
constexpr size_t kBufferSize{4096};

enum class CalculationType : uint8_t {
  Plus = 0,
  Minus = 1,
  Multiplication = 2,
  Division = 3,
};

struct ServerFunctions {
  using bool_functions = std::function<bool(bool, bool)>;

  template <std::floating_point T>
  static T fpCalculation(CalculationType type, T value1, T value2)
  {
    logging::logger_presets::functionCall();
    T result{};
    switch (type) {
      case CalculationType::Plus:
        result = value1 + value2;
        break;
      case CalculationType::Minus:
        result = value1 - value2;
        break;
      case CalculationType::Multiplication:
        result = value1 * value2;
        break;
      case CalculationType::Division:
        result = value1 / value2;
    }
    return result;
  }

  template <std::integral T>
  static T integerCalc(CalculationType type, T value1, T value2)
  {
    logging::logger_presets::functionCall();
    T result{};
    switch (type) {
      case CalculationType::Plus:
        result = value1 + value2;
        break;
      case CalculationType::Minus:
        result = value1 - value2;
        break;
      case CalculationType::Multiplication:
        result = value1 * value2;
        break;
      case CalculationType::Division:
        result = value2 != 0 ? value1 / value2 : 0;
    }
    return result;
  }

  static std::array<bool_functions, 4> getBoolFunc()
  {
    logging::logger_presets::functionCall();
    return std::array<bool_functions, 4>{
        [](bool a1, bool a2) { return a1 || a2; },
        [](bool a1, bool a2) { return a1 && a2; },
        [](bool a1, bool a2) { return (!a2) || (!a1); },
        [](bool a1, bool a2) { return (!a1) && (!a2); },
    };
  }
};

static std::expected<uint16_t, bool> parsePortServer(std::string_view port)
{
  logging::logger_presets::functionCall();
  uint16_t port_num = kPortNum;

  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_num);
  if (ec != std::errc() || ptr != port.end()) {
    logging::logger_presets::userInputError(port, *ptr);
    return std::unexpected(true);
  }

  return port_num;
}

static std::shared_ptr<int> serverSetup(uint16_t port)
{
  logging::logger_presets::functionCall();

  std::shared_ptr<int> server_socket(new int(-1), [](const int* ptr) {
    if (*ptr != -1) {
      close(*ptr);
    };
    //NOLINTNEXTLINE
    delete ptr;
  });

  *server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (*server_socket == -1) {
    logging::logger_presets::defaultError("Couldn't init server socket\n");
    return nullptr;
  }

  sockaddr_in server_addr{
      .sin_family = AF_INET, .sin_port = htons(port), .sin_addr{}, .sin_zero{}};

  server_addr.sin_addr.s_addr = INADDR_ANY;

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  if (bind(*server_socket, reinterpret_cast<sockaddr*>(&server_addr),
           sizeof(server_addr)) == -1) {
    logging::logger_presets::defaultError("Couldn't init server socket\n");
    return nullptr;
  }

  if (listen(*server_socket, 2) == -1) {
    logging::logger_presets::defaultError("Couldn't init server socket\n");
    return nullptr;
  }
  return server_socket;
}

static void dataManipulation(std::string& result,
                             custom_types::PolymorphicVectorQuad& vector)
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
              logic = bool_functions.at(index_function_map++)(
                  std::get<bool>(*vector.begin()), logic);
            },
            [](std::string& str) {
              std::ranges::transform(str, str.begin(), ::toupper);
            }},
        element);

    std::visit(custom_types::Visitor{[&result](const auto& element) {
                 result += std::format("{} ", element);
               }},
               element);
  }
}
//=============================

int main(int argc, char* argv[])
{
  logging::Logger::loggerInit("LogsServer");

  auto span_argv = std::span(argv, static_cast<size_t>(argc));
  span_argv = span_argv.subspan(1);
  argc--;

  if (argc > 1) {
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

  std::shared_ptr<int> server_socket = serverSetup(port.value());
  if (server_socket == nullptr) {
    logging::logger_presets::defaultError("Couldn't create socket");
    return -1;
  }

  std::shared_ptr<int> client_socket(new int(-1), [](const int* ptr) {
    if (*ptr != -1) {
      close(*ptr);
    };
    //NOLINTNEXTLINE
    delete ptr;
  });
  std::string str_buff;
  str_buff.resize(kBufferSize);

  //auto size_addr = static_cast<socklen_t>(sizeof(server_addr));
  nlohmann::json jsn{};

  while (true) {
    std::ranges::fill(str_buff, 0);

    *client_socket = accept(*server_socket, nullptr, nullptr);

    if (*client_socket == -1) {
      logging::logger_presets::defaultError("Couldn't init client socket\n");
      return -1;
    }

    auto symbols_read =
        recv(*client_socket, str_buff.data(), str_buff.length(), 0);

    if (symbols_read == 0) {
      logging::Logger::writeToLog<config::LogVerbosity::Warning>(
          "Client disconnected");
      close(*client_socket);
      continue;
    }

    if (symbols_read == -1) {
      logging::Logger::writeToLogNCl<config::LogVerbosity::Error>("Read Error");
      continue;
    }

    jsn = nlohmann::json::parse(str_buff.begin(), str_buff.end());

    logging::Logger::writeToLogNCl<config::LogVerbosity::Info>(
        std::format("{}", jsn.dump(4)));

    custom_types::PolymorphicVectorQuad vector =
        parsing::parseStringVector(jsn);

    std::string result;
    dataManipulation(result, vector);

    jsn["Vector"] = result;
    result = jsn.dump();

    if (send(*client_socket, result.data(), result.length(), 0) == -1) {
      logging::Logger::writeToLogNCl<config::LogVerbosity::Error>(
          "Coudn't send answer back");
      continue;
    }
  }
}

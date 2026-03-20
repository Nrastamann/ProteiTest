#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <concepts>
#include <expected>
#include <functional>
#include <iterator>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include "custom_types.hpp"
#include "logger.hpp"

namespace server {

inline constexpr uint16_t kPortNum{5001};
inline constexpr size_t kBufferSize{4096};

enum class CalculationType : uint8_t {
  Plus = 0,
  Minus = 1,
  Multiplication = 2,
  Division = 3,
};

struct SocketWrapper {
  int _socket{};
  SocketWrapper() = default;
  ~SocketWrapper()
  {
    if (_socket != -1)
      close(_socket);
  }
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
        result =
            value2 != static_cast<T>(0) ? static_cast<T>(value1 / value2) : static_cast<T>(0);
    }
    return result;
  }

  static std::array<bool_functions, 4>& getBoolFunc()
  {
    logging::logger_presets::functionCall();
    static std::array<bool_functions, 4> functions{
        [](bool a1, bool a2) { return a1 || a2; },
        [](bool a1, bool a2) { return a1 && a2; },
        [](bool a1, bool a2) { return (!a2) || (!a1); },
        [](bool a1, bool a2) { return (!a1) && (!a2); },
    };

    return functions;
  }
};

inline std::atomic<bool>& getServerRunning()
{
  static std::atomic<bool> server_running{true};
  return server_running;
}

static constexpr size_t kThreadNum{8};

template <size_t N>
class BufferPool {
  template <typename T>
  using poolContainer = std::array<std::pair<std::atomic<bool>, T>, N>;

 public:
  using BufferPair = std::pair<std::span<char, kBufferSize>, nlohmann::json>;

  BufferPair getBuffer(size_t& idx)
  {
    auto* it_jsons = _jsons.begin();
    const auto* it_end = _jsons.end();

    for (auto& buffers : _buffers) {
      if (!buffers.first) {
        buffers.first = true;
        it_jsons->first = true;
        return std::pair{std::span{buffers.second}, it_jsons->second};
      }
      std::advance(it_jsons, 1);
      idx = static_cast<size_t>(it_end - it_jsons);
    }
    return std::pair{std::span{_buffers.at(idx).second}, _buffers.at(idx).second};
  }
  void resetBuffer(size_t idx) { _buffers[idx].first = false; }

 private:
  poolContainer<std::array<char, kBufferSize>> _buffers;
  poolContainer<nlohmann::json> _jsons;
};

std::expected<uint16_t, bool> parsePortServer(std::string_view port);
SocketWrapper serverSetup(uint16_t port);
void dataManipulation(std::string& result, custom_types::PolymorphicVectorQuad& vector);
void serverTask(int client_socket, BufferPool<kThreadNum>& buffer_pool);
int serverStart(int argc, char** argv);
}  // namespace server

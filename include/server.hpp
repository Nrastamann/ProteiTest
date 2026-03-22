#pragma once
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <array>
#include <concepts>
#include <condition_variable>
#include <expected>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>
#include <string_view>
#include <type_traits>
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
    logging::MultithreadPresets::functionCall();
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
    logging::MultithreadPresets::functionCall();
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
    logging::MultithreadPresets::functionCall();
    static std::array<bool_functions, 4> functions{
        [](bool a1, bool a2) { return a1 || a2; },
        [](bool a1, bool a2) { return a1 && a2; },
        [](bool a1, bool a2) { return (!a2) || (!a1); },
        [](bool a1, bool a2) { return (!a1) && (!a2); },
    };

    return functions;
  }
};

class CloseTag {};
class SetTag {};

template <typename Tag>
inline int setGetServerSocket(Tag, int socket = 0)
{
  static int global_server_socket;
  if constexpr (std::is_same_v<Tag, SetTag>) {
    global_server_socket = socket;
    return 0;
  }

  return global_server_socket;
}

inline constexpr size_t kThreadNum{4};

template <size_t N>
class BufferPool {
  template <typename T>
  using poolContainer = std::array<T, N>;
  struct BufferIndexWrapper {
    BufferIndexWrapper(BufferPool<N>& queue, uint8_t index) : _queue_ref(queue), _index(index)
    {
    }
    operator size_t() { return static_cast<size_t>(_index); }
    operator uint8_t() { return _index; }
    ~BufferIndexWrapper() { _queue_ref.pushBuffer(_index); }
    BufferPool<N>& _queue_ref;
    uint8_t _index;
  };

 public:
  BufferPool()
  {
    for (uint8_t i = 0; static_cast<size_t>(i) < N; ++i) {
      _indexes_queue.push(i);
    }
  }
  BufferIndexWrapper getBuffer()
  {
    logging::MultithreadPresets::functionCall();
    std::unique_lock<std::mutex> lock(_queue_mtx);
    _cv.wait(lock, [this]() { return !_indexes_queue.empty() || _quit; });

    if (!_indexes_queue.empty()) [[likely]] {
      uint8_t index = _indexes_queue.front();
      _indexes_queue.pop();

      lock.unlock();
      _cv.notify_one();
      return BufferIndexWrapper{*this, index};
    }
    return BufferIndexWrapper{*this, 0};
  }
  void pushBuffer(uint8_t index)
  {
    std::lock_guard<std::mutex> lock(_queue_mtx);
    _indexes_queue.push(index);
  }
  void resetBuffer(size_t idx) { _buffers[idx].first = false; }

  poolContainer<std::array<char, kBufferSize>> _buffers;
  poolContainer<nlohmann::json> _jsons;

 private:
  std::mutex _queue_mtx;
  std::condition_variable _cv;
  std::atomic<bool> _quit;
  std::queue<uint8_t> _indexes_queue;
};

std::expected<uint16_t, bool> parsePortServer(std::string_view port);
SocketWrapper serverSetup(uint16_t port);
void dataManipulation(std::string& result, custom_types::PolymorphicVectorQuad& vector);
void serverTask(int client_socket, BufferPool<kThreadNum>& buffer_pool);
int serverStart(int argc, char** argv);

struct WriteSocketN {};
struct GetSocketN {};

template <typename T>
inline int getSetServerSocket(T, int socket = 0)
{
  static int socket_server;
  if constexpr (std::is_same_v<T, WriteSocketN>) {
    socket_server = socket;
    return 0;
  }
  else {
    return socket_server;
  }
}
}  // namespace server

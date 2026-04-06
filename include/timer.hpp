#pragma once

#include <chrono>
namespace pr_utils {
class Timer {
 public:
  explicit Timer(uint64_t ms_time_to_wait)
      : _start_time(std::chrono::system_clock::now()), _ms_to_wait(ms_time_to_wait)
  {
  }

  void restart() { _start_time = std::chrono::system_clock::now(); }

  bool checkTimer()
  {
    auto current_time = std::chrono::system_clock::now();
    bool is_running =
        _ms_to_wait >=
        static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(current_time - _start_time)
                .count());

    return is_running;
  }

 private:
  std::chrono::time_point<std::chrono::system_clock> _start_time;
  uint64_t _ms_to_wait{};
};
}  // namespace pr_utils

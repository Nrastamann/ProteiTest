#pragma once

#include <format>
#include <queue>

#include "logger.hpp"

namespace data_storage {
class DataPool {
  using value_type = std::string;
  using return_type = std::string;
  using return_reference_type = return_type&;

  using const_return_reference_type = const return_type&;

 public:
  [[nodiscard]] size_t size() const { return _queue.size(); }
  return_reference_type back() { return _queue.back(); }

  template <typename T>
  void push(T&& vec)
  {
    logging::SingleThreadPresets::containerPush<DataPool>(std::format("{}", vec));
    _queue.push(std::forward<T>(vec));
  }

  return_reference_type front() { return _queue.front(); }
  [[nodiscard]] const_return_reference_type front() const { return _queue.front(); }

  void pop()
  {
    logging::SingleThreadPresets::containerRemove<DataPool>(std::format("{}", _queue.front()));
    _queue.pop();
  }

 private:
  std::vector<value_type> _queue;
};
}  // namespace data_storage

#pragma once

#include <format>
#include <string>
#include "logger.hpp"

namespace data_storage {
class DataPool {
  using value_type = std::string;
  using return_type = std::string;
  using return_reference_type = return_type&;

  using const_return_reference_type = const return_type&;
  using container_type = std::vector<value_type>;
  using container_type_ref = container_type&;

 public:
  [[nodiscard]] size_t size() const { return _storage.size(); }
  return_reference_type back() { return _storage.back(); }

  template <typename T>
  void push(T&& vec)
  {
    logging::SingleThreadPresets::containerPush<DataPool>(std::format("{}", vec));
    _storage.push_back(std::forward<T>(vec));
  }

  return_reference_type front() { return _storage.front(); }
  [[nodiscard]] const_return_reference_type front() const { return _storage.front(); }

  void pop()
  {
    logging::SingleThreadPresets::containerRemove<DataPool>(
        std::format("{}", _storage.front()));
    _storage.pop_back();
  }
  container_type_ref getContainer() { return _storage; }
  [[nodiscard]] container_type getContainer() const { return _storage; }

 private:
  container_type _storage;
};
}  // namespace data_storage

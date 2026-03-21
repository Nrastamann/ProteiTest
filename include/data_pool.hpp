#pragma once

#include <format>
#include <queue>

#include "custom_types.hpp"
#include "logger.hpp"

namespace data_storage {
struct PolymorphicDimensionalVector {
  ~PolymorphicDimensionalVector() = default;
  PolymorphicDimensionalVector(const PolymorphicDimensionalVector&) = default;
  PolymorphicDimensionalVector(PolymorphicDimensionalVector&&) = default;
  PolymorphicDimensionalVector& operator=(const PolymorphicDimensionalVector&) = default;
  PolymorphicDimensionalVector& operator=(PolymorphicDimensionalVector&&) = default;

  PolymorphicDimensionalVector(custom_types::PolymorphicVectorQuad vec, size_t type_hash)
      : _vec(std::move(vec)), _type_hash(type_hash)
  {
    logging::SingleThreadPresets::createObject<PolymorphicDimensionalVector>();
  }

  custom_types::PolymorphicVectorQuad _vec;
  size_t _type_hash;
};
}  // namespace data_storage

template <>
struct std::formatter<data_storage::PolymorphicDimensionalVector>
    : std::formatter<std::string> {
  auto format(const data_storage::PolymorphicDimensionalVector& vec,
              std::format_context& ctx) const
  {
    std::string out;
    size_t type = vec._type_hash;
    for (const auto& i : vec._vec) {
      std::visit(custom_types::Visitor{[&out](auto const& variant_val) {
                   out += std::format("{} ", variant_val);
                 }},
                 i);
    }
    out += std::format("{}", type);
    return std::formatter<std::string>::format(out, ctx);
  }
};

namespace data_storage {
class DataPool {
  using return_type = PolymorphicDimensionalVector;
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
  std::queue<PolymorphicDimensionalVector> _queue;
};
}  // namespace data_storage

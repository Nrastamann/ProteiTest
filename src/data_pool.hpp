#pragma once

#include <format>
#include <queue>
#include <utility>
#include "logger.hpp"
#include "utility.hpp"

struct PolymorphicDimensionalVector {
  ~PolymorphicDimensionalVector() = default;
  PolymorphicDimensionalVector(const PolymorphicDimensionalVector&) = default;
  PolymorphicDimensionalVector(PolymorphicDimensionalVector&&) = default;
  PolymorphicDimensionalVector& operator=(const PolymorphicDimensionalVector&) =
      default;
  PolymorphicDimensionalVector& operator=(PolymorphicDimensionalVector&&) =
      default;

  PolymorphicDimensionalVector(ProteiVector vec, size_t type_hash)
      : _vec(std::move(vec)), _type_hash(type_hash)
  {
    logger_presets::createObject<PolymorphicDimensionalVector>();
  }

  ProteiVector _vec;
  size_t _type_hash;
};

template <>
struct std::formatter<PolymorphicDimensionalVector>
    : std::formatter<std::string> {
  auto format(const PolymorphicDimensionalVector& vec,
              std::format_context& ctx) const
  {
    std::string out;
    size_t type = vec._type_hash;
    for (const auto& i : vec._vec) {
      std::visit(Visitor{[&out](auto const& variant_val) {
                   out += std::format("{} ", variant_val);
                 }},
                 //  [&out](int8_t value) { out << +value << ' '; },
                 //  [&out](uint8_t value) { out << +value << ' '; }},
                 i);
    }
    out += std::format("{}", type);
    return std::formatter<std::string>::format(out, ctx);
  }
};

class DataPool {
  using return_type = PolymorphicDimensionalVector;
  using return_reference_type = return_type&;

  using const_return_reference_type = const return_type&;

 public:
  [[nodiscard]] size_t size() const { return _queue.size(); }
  return_reference_type back() { return _queue.back(); }

  void push(PolymorphicDimensionalVector&& vec)
  {
    logger_presets::containerPush<DataPool>(std::format("{}", vec));
    _queue.push(std::move(vec));
  }

  return_reference_type front() { return _queue.front(); }
  [[nodiscard]] const_return_reference_type front() const
  {
    return _queue.front();
  }

  void pop()
  {
    logger_presets::containerRemove<DataPool>(
        std::format("{}", _queue.front()));
    _queue.pop();
  }

 private:
  std::queue<PolymorphicDimensionalVector> _queue;
};

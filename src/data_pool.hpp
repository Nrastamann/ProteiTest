#pragma once

#include <format>
#include <queue>
#include <utility>
#include "config.h"
#include "logger.h"
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
  }

  ProteiVector _vec;
  size_t _type_hash;
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
    Logger::writeToLog(config::LogVerbosity::Debug, "Pushing to DataPool");

    _queue.push(std::move(vec));
  }

  return_reference_type front() { return _queue.front(); }
  [[nodiscard]] const_return_reference_type front() const
  {
    return _queue.front();
  }

  void pop()
  {
    Logger::writeToLog(config::LogVerbosity::Debug, "Poping from DataPool");
    _queue.pop();
  }

 private:
  std::queue<PolymorphicDimensionalVector> _queue;
};

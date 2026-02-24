#pragma once
#include <cstdint>
#include <iostream>
#include <variant>

#include "settings.hpp"

static constexpr size_t kVectorDimensionsAmount{4};

using any_type =
    std::variant<int, float, double, char, std::string, bool, int8_t, int16_t,
                 int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

template <size_t N>
using PolymorpicVector = std::array<any_type, N>;

void changeType(Settings& settings);
void changeRole(Settings& settings);

void enterVector(PolymorpicVector<kVectorDimensionsAmount>& vector,
                 Settings const& settings);

inline void printVector(PolymorpicVector<kVectorDimensionsAmount>& arr)
{
  for (auto& i : arr) {
    std::visit([](auto const& variant_val) { std::cout << variant_val << ' '; },
               i);
  }
  std::cout << '\n';
}

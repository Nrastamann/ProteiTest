#pragma once
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <variant>

#include "settings.hpp"
#include "static_containers.hpp"

static constexpr size_t kVectorDimensionsAmount{4};

using any_type =
    std::variant<float, double, char, std::string, bool, int8_t, int16_t,
                 int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

template <size_t N>
using PolymorphicVector = std::array<any_type, kVectorDimensionsAmount>;

using ProteiVector = PolymorphicVector<kVectorDimensionsAmount>;

const std::unordered_map<static_containers::EnumTypes, any_type>&
getDefaultValues();

void changeType(Settings& settings);
void changeRole(Settings& settings);

void enterVector(PolymorphicVector<kVectorDimensionsAmount>& vector,
                 Settings const& settings);

inline void quit(Settings& settings)
{
  settings.setShouldClose();
}

inline void printCurrentSettings(Settings& settings)
{
  std::cout << settings;
}

inline void printVector(const PolymorphicVector<kVectorDimensionsAmount>& arr)
{
  for (const auto& i : arr) {
    std::visit([](auto const& variant_val) { std::cout << variant_val << ' '; },
               i);
  }
  std::cout << '\n';
}
inline void wrongOption()
{
  std::cout << "Wrong menu option, try again\n";
}

#pragma once
#include <cstdint>
#include <string>
#include <variant>

template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};

static constexpr size_t kVectorDimensionsAmount{4};

using any_type =
    std::variant<float, double, char, std::string, bool, int8_t, int16_t,
                 int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

template <size_t N>
using PolymorphicVector = std::array<any_type, kVectorDimensionsAmount>;

using ProteiVector = PolymorphicVector<kVectorDimensionsAmount>;

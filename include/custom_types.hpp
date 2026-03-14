#pragma once
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace hashed {
inline size_t const kInt = std::hash<std::string_view>{}("int");
inline size_t const kFloat = std::hash<std::string_view>{}("float");
inline size_t const kDouble = std::hash<std::string_view>{}("double");
inline size_t const kChar = std::hash<std::string_view>{}("char");
inline size_t const kString = std::hash<std::string_view>{}("string");
inline size_t const kBool = std::hash<std::string_view>{}("bool");

inline size_t const kInt8 = std::hash<std::string_view>{}("int8_t");
inline size_t const kInt16 = std::hash<std::string_view>{}("int16_t");
inline size_t const kInt32 = std::hash<std::string_view>{}("int32_t");
inline size_t const kInt64 = std::hash<std::string_view>{}("int64_t");

inline size_t const kUInt8 = std::hash<std::string_view>{}("uint8_t");
inline size_t const kUInt16 = std::hash<std::string_view>{}("uint16_t");
inline size_t const kUInt32 = std::hash<std::string_view>{}("uint32_t");
inline size_t const kUInt64 = std::hash<std::string_view>{}("uint64_t");
}  // namespace hashed

namespace protei_types {
enum class EnumTypes : uint8_t {
  Int,
  Float,
  Double,
  Char,
  String,
  Bool,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
};

enum class MenuOptions : uint8_t {
  ChangeRole,
  ChangeType,
  PrintSettings,
  EnterVector,
  PrintCurrentVector,
  QuitProgram,
  WrongOption,
  EmptyQueue,
};

std::unordered_map<size_t, std::pair<EnumTypes, std::string_view>> const&
getHashToTypeInfo();
std::unordered_map<size_t, MenuOptions> const& getMenuOptions();

template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};

static constexpr size_t kVectorDimensionsAmount{4};

using any_type =
    std::variant<float, double, char, std::string, bool, int8_t, int16_t,
                 int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

template <size_t N>
using PolymorphicVector =
    std::array<protei_types::any_type, kVectorDimensionsAmount>;

using ProteiVector = PolymorphicVector<kVectorDimensionsAmount>;
};  // namespace protei_types

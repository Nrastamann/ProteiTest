#pragma once
#include <string_view>
namespace hashed {
inline size_t const kQuit = std::hash<std::string_view>{}("quit");

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

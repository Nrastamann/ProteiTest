#pragma once
#include <string_view>
namespace hashed {
inline size_t const kAddrHash = {std::hash<std::string_view>{}("-a")};
inline size_t const kPortHash = {std::hash<std::string_view>{}("-p")};
inline size_t const kRoleHash = {std::hash<std::string_view>{}("-r")};
inline size_t const kIndexHash = {std::hash<std::string_view>{}("-i")};
inline size_t const kLibHash = {std::hash<std::string_view>{}("-L")};

inline size_t const kRoleMenuHash = std::hash<std::string_view>{}("role");
inline size_t const kTypeMenuHash = std::hash<std::string_view>{}("type");
inline size_t const kVectorMenuHash = std::hash<std::string_view>{}("vector");
inline size_t const kPrintHash = std::hash<std::string_view>{}("print");
inline size_t const kQuitMenuHash = std::hash<std::string_view>{}("quit");
inline size_t const kSettingsMenuHash =
    std::hash<std::string_view>{}("settings");

inline size_t const kIntHash = std::hash<std::string_view>{}("int");
inline size_t const kFloatHash = std::hash<std::string_view>{}("float");
inline size_t const kDoubleHash = std::hash<std::string_view>{}("double");
inline size_t const kCharHash = std::hash<std::string_view>{}("char");
inline size_t const kStringHash = std::hash<std::string_view>{}("string");
inline size_t const kBoolHash = std::hash<std::string_view>{}("bool");

inline size_t const kInt8Hash = std::hash<std::string_view>{}("int8_t");
inline size_t const kInt16Hash = std::hash<std::string_view>{}("int16_t");
inline size_t const kInt32Hash = std::hash<std::string_view>{}("int32_t");
inline size_t const kInt64Hash = std::hash<std::string_view>{}("int64_t");

inline size_t const kUInt8Hash = std::hash<std::string_view>{}("uint8_t");
inline size_t const kUInt16Hash = std::hash<std::string_view>{}("uint16_t");
inline size_t const kUInt32Hash = std::hash<std::string_view>{}("uint32_t");
inline size_t const kUInt64Hash = std::hash<std::string_view>{}("uint64_t");

inline size_t const kTrueHashSymbolic = std::hash<std::string_view>{}("true");
inline size_t const kFalseHashSymbolic = std::hash<std::string_view>{}("false");
}  // namespace hashed

#pragma once
#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace static_containers {
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
};  // namespace static_containers

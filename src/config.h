#pragma once
#include <cstdint>
namespace config {
enum class LogVerbosity : uint8_t {
  Error = 0,
  Warning = 1,
  Info = 2,
  Debug = 3,
  Trace = 4
};

inline constexpr LogVerbosity kLogVerbosity = LogVerbosity::Trace;
}  // namespace config

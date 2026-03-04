#pragma once

#include <cstdint>
#include <string_view>
namespace config {

enum class LogVerbosity : uint8_t {
  Error = 0,
  Warning = 1,
  Info = 2,
  Debug = 3,
  Trace = 4
};

std::string_view toStr(LogVerbosity verbosity);

constexpr LogVerbosity kLogVerbosity{LogVerbosity::Trace};

}  // namespace config

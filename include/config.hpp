#pragma once

#include <cstdint>
#include <string_view>
namespace config {

enum class LogVerbosity : uint8_t {
  NOLOG = 0,
  Error = 1,
  Warning = 2,
  Info = 3,
  Debug = 4,
  Trace = 5
};

static constexpr std::string_view toStr(const LogVerbosity verbosity)
{
  switch (verbosity) {
    case LogVerbosity::Trace:
      return "Trace";
    case LogVerbosity::Debug:
      return "Debug";
    case LogVerbosity::Info:
      return "Info";
    case LogVerbosity::Error:
      return "Error";
    case LogVerbosity::Warning:
      return "Warning";
    default:
      return "NotImplemented";
  }
}
}  // namespace config

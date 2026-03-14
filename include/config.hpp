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
  }
}

constexpr LogVerbosity kLogVerbosity
{
  LogVerbosity::Trace
};

}  // namespace config

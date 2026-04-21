#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string_view>
namespace config {
namespace hashed {
const size_t kTrace{std::hash<std::string_view>{}("trace")};
const size_t kDebug{std::hash<std::string_view>{}("debug")};
const size_t kInfo{std::hash<std::string_view>{}("info")};
const size_t kError{std::hash<std::string_view>{}("error")};
const size_t kWarning{std::hash<std::string_view>{}("warning")};
}  // namespace hashed
enum class LogVerbosity : uint8_t {
  WRONG_FLAG = 0,
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
inline LogVerbosity fromStr(std::string_view str)
{
  size_t hash = std::hash<std::string_view>{}(str);

  std::map<size_t, LogVerbosity> verbosity_map{
      {
          hashed::kTrace,
          LogVerbosity::Trace,
      },
      {
          hashed::kDebug,
          LogVerbosity::Debug,
      },
      {
          hashed::kInfo,
          LogVerbosity::Info,
      },
      {
          hashed::kWarning,
          LogVerbosity::Warning,
      },
      {
          hashed::kError,
          LogVerbosity::Error,
      },
  };
  auto it = verbosity_map.find(hash);
  return it == verbosity_map.end() ? LogVerbosity::WRONG_FLAG : it->second;
}
}  // namespace config

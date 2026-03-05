#include "config.hpp"
namespace config {
std::string_view toStr(LogVerbosity verbosity)
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
}  // namespace config

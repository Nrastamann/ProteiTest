#include "logger.hpp"
#include <format>
#include <source_location>
#include <string_view>
#include "config.h"

template <typename T>
static constexpr std::string_view acquireName()
{
  constexpr auto kDeclaration = __PRETTY_FUNCTION__;

  constexpr auto kStart = kDeclaration.find("T = ") + 4;

  constexpr auto kEnd = [&] {
    if constexpr (kDeclaration.find(']') != std::string_view::npos)
      return kDeclaration.find(']', kStart);  // GCC/Clang
    else
      return kDeclaration.find('>', kStart);  // MSVC fallback
  }();

  return kDeclaration.substr(kStart, kEnd - kStart);
}

namespace logger_presets {
inline void userInputError(std::string_view input_str, char symbol,
                           std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error during input {} at the {} symbol", input_str, symbol),
      loc);
}

inline void parsingInputError(std::string_view input_str, char symbol,
                              std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error during parsing {} at the {} symbol", input_str,
                  symbol),
      loc);
}
inline void defaultError(std::string_view metadata, std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error at function {} - metadata {}", loc.function_name(),
                  metadata),
      loc);
}
inline void createdStaticContainer(std::string_view description,
                                   std::source_location loc)
{
  Logger::writeToLog<config::LogVerbosity::Trace>(
      std::format("Created static container {}", description), loc);
}

inline void functionCall(std::source_location loc)
{
  Logger::writeToLog<config::LogVerbosity::Trace>(
      std::format("Function called"), loc);
}

}  // namespace logger_presets

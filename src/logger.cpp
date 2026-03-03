#include "logger.hpp"
#include <format>
#include <source_location>
#include <string_view>
#include "config.h"

namespace logger_presets {
void userInputError(std::string_view input_str, char symbol,
                    std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error during input {} at the {} symbol", input_str, symbol),
      loc);
}

void parsingInputError(std::string_view input_str, char symbol,
                       std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error during parsing {} at the {} symbol", input_str,
                  symbol),
      loc);
}
void defaultError(std::string_view metadata, std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error at function {} - metadata {}", loc.function_name(),
                  metadata),
      loc);
}
void createdStaticContainer(std::string_view description,
                            std::source_location loc)
{
  Logger::writeToLog<config::LogVerbosity::Trace>(
      std::format("Get static container {}", description), loc);
}

void functionCall(std::source_location loc)
{
  Logger::writeToLog<config::LogVerbosity::Trace>("Function called", loc);
}

void menuQuit(std::source_location loc)
{
  Logger::writeToLog<config::LogVerbosity::Trace>("Menu function quit", loc);
}
void wrongInput(std::source_location loc)
{
  Logger::writeToLogNCl<config::LogVerbosity::Warning>("Wrong input, try again",
                                                       loc);
}
void userInput(std::string_view input, std::source_location loc)
{
  Logger::writeToLog<config::LogVerbosity::Debug>(
      std::format("input - {}", input), loc);
}
}  // namespace logger_presets

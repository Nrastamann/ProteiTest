#pragma once
#include <cxxabi.h>
#include <chrono>
#include <concepts>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include "config.hpp"
namespace logging {

struct DefaultPolicy {};
struct MultithreadingPolicy : DefaultPolicy {};
struct SingleThreadPolicy : DefaultPolicy {};

template <typename DerivedClass, typename BaseClass>
concept Derived = std::derived_from<DerivedClass, BaseClass>;
template <typename DerivedClass>
concept LoggerPolicy = Derived<DerivedClass, DefaultPolicy>;

template <typename T>
static std::unique_ptr<char, void (*)(void*)> acquireName()
{
  return {abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr), std::free};
}

template <LoggerPolicy Policy>
class Logger {
  template <config::LogVerbosity LogLevel>
  static void writeImpl(std::string_view str, std::source_location loc, SingleThreadPolicy)
  {
    std::ofstream file(log_file, std::ios::app);

    if (!file.is_open()) {
      std::cout << "Couldn't open the log_file!\n";
      return;
    }
    file << std::format("[{}]: <{}> [{} : {} : {} : {}] | {}\n",
                        std::chrono::system_clock::now(), config::toStr(LogLevel),
                        std::this_thread::get_id(), loc.file_name(), loc.function_name(),
                        loc.line(), str);

    file.close();
  }

  template <config::LogVerbosity LogLevel>
  static void writeImpl(std::string_view str, std::source_location loc, MultithreadingPolicy)
  {
    std::string output_str =
        std::format("[{}]: <{}> [{} : {} : {} : {}] | {}\n", std::chrono::system_clock::now(),
                    config::toStr(LogLevel), std::this_thread::get_id(), loc.file_name(),
                    loc.function_name(), loc.line(), str);

    std::lock_guard<std::mutex> lock(log_access);

    std::ofstream file(log_file, std::ios::app);

    if (!file.is_open()) {
      std::cout << "Couldn't open the log_file!\n";
      return;
    }

    file << output_str;
    file.close();
  }

 public:
  template <config::LogVerbosity LogLevel>
  static void writeToLog(std::string_view str,
                         std::source_location loc = std::source_location::current())
  {
    if (!log_on || LogLevel > verbosity) {
      return;
    }
    writeImpl<LogLevel>(str, loc, Policy{});
  }

  template <config::LogVerbosity LogLevel>
  static void writeToLogNCl(std::string_view str,
                            std::source_location loc = std::source_location::current())
  {
    if (verbosity <= LogLevel) {
      return;
    }
    std::cout << str << '\n';
    writeToLog<LogLevel>(str, loc);
  }

  struct LoggerPresets {
    template <typename Type>
    static void acquiringResourceError(
        std::string_view metadata, std::source_location loc = std::source_location::current())
    {
      auto ptr = acquireName<Type>();

      Logger::writeToLogNCl<config::LogVerbosity::Error>(
          std::format("Error during acquiring {} with metadata {}", ptr.get(), metadata), loc);
    }
    template <typename Type>
    static void containerRemove(std::string_view metadata,
                                std::source_location loc = std::source_location::current())
    {
      auto ptr = acquireName<Type>();

      Logger::writeToLog<config::LogVerbosity::Info>(
          std::format("Removed object from {} - metadata {}", ptr.get(), metadata), loc);
    }
    template <typename Type>
    static void containerPush(std::string_view metadata,
                              std::source_location loc = std::source_location::current())
    {
      auto ptr = acquireName<Type>();

      Logger::writeToLog<config::LogVerbosity::Info>(
          std::format("Pushed object to {} - metadata {}", ptr.get(), metadata), loc);
    }

    template <typename Type>
    static void createObject(std::source_location loc = std::source_location::current())
    {
      auto ptr = acquireName<Type>();

      Logger::writeToLog<config::LogVerbosity::Info>(
          std::format("Starting {} creation", ptr.get()), loc);
    }

    static void userInputError(std::string_view input_str, char symbol,
                               std::source_location loc = std::source_location::current())
    {
      Logger::writeToLogNCl<config::LogVerbosity::Error>(
          std::format("Error during input {} at the {} symbol", input_str, symbol), loc);
    }
    static void parsingInputError(std::string_view input_str, char symbol,
                                  std::source_location loc = std::source_location::current())
    {
      Logger::writeToLogNCl<config::LogVerbosity::Error>(
          std::format("Error during parsing {} at the {} symbol", input_str, symbol), loc);
    }
    static void defaultError(std::string_view metadata,
                             std::source_location loc = std::source_location::current())
    {
      Logger::writeToLogNCl<config::LogVerbosity::Error>(
          std::format("Error at function {} - metadata {}", loc.function_name(), metadata),
          loc);
    }
    static void createdStaticContainer(
        std::string_view description,
        std::source_location loc = std::source_location::current())
    {
      Logger::writeToLog<config::LogVerbosity::Trace>(
          std::format("Get static container {}", description), loc);
    }

    static void functionCall(std::source_location loc = std::source_location::current())
    {
      Logger::writeToLog<config::LogVerbosity::Trace>("Function called", loc);
    }

    static void menuQuit(std::source_location loc = std::source_location::current())
    {
      Logger::writeToLog<config::LogVerbosity::Trace>("Menu function quit", loc);
    }
    static void wrongInput(std::source_location loc = std::source_location::current())
    {
      Logger::writeToLogNCl<config::LogVerbosity::Warning>("Wrong input, try again", loc);
    }
    static void userInput(std::string_view input,
                          std::source_location loc = std::source_location::current())
    {
      Logger::writeToLog<config::LogVerbosity::Debug>(std::format("input - {}", input), loc);
    }
  };

  static void loggerInit(const std::string& path = "logs",
                         config::LogVerbosity log_level = config::LogVerbosity::Info)
  {

    verbosity = log_level;
    log_file = path;
    log_on = true;

    if (!std::filesystem::exists(log_file)) {
      bool dir_created = std::filesystem::create_directory(log_file);
      if (!dir_created) {
        log_on = false;
        return;
      }
    }
    auto time = std::chrono::system_clock::now();

    std::string time_str = std::format("{}", time);
    log_file += "/log-";

    auto t = std::chrono::system_clock::to_time_t(time);
    auto str = std::string_view(std::ctime(&t));
    str.remove_suffix(1);
    log_file += str;
    log_file += time_str.substr(time_str.size() - 4);

    std::ofstream file(log_file);
    if (file.is_open()) {
      file.close();
    }
  }

  Logger() = default;
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger& operator=(Logger&&) = delete;

 private:
  inline static std::string log_file;
  inline static config::LogVerbosity verbosity;
  inline static std::mutex log_access;
  inline static bool log_on{false};
};

using MultithreadLogger = Logger<MultithreadingPolicy>;
using SingleThreadLogger = Logger<MultithreadingPolicy>;

using MultithreadPresets = MultithreadLogger::LoggerPresets;
using SingleThreadPresets = SingleThreadLogger::LoggerPresets;

}  // namespace logging

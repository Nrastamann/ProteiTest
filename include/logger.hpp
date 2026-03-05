#pragma once
#include <cxxabi.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <thread>
#include "config.hpp"  //!!!

template <typename T>
std::unique_ptr<char, void (*)(void*)> acquireName()
{
  return {abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr),
          std::free};
}

class Logger {

 public:
  static void loggerInit(const std::string& path = "logs")
  {
    log_file = path;
    if (!std::filesystem::exists(log_file)) {
      std::filesystem::create_directory(log_file);
    }

    log_file += "/log-";
    auto t =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto str = std::string_view(std::ctime(&t));
    str.remove_suffix(1);
    log_file += str;
    std::ofstream file(log_file);
    if (file.is_open()) {
      file.close();
    }
  }

  template <config::LogVerbosity LogLevel>
  static void writeToLog(
      std::string_view str,
      std::source_location loc = std::source_location::current())
  {
    if (LogLevel > config::kLogVerbosity) {
      return;
    }
    std::ofstream file(log_file, std::ios::app);
    if (!file.is_open()) {
      std::cout << "Couldn't open the log_file!\n";
      return;
    }

    file << std::format("[{}]: <{}> [{} : {} : {} : {}] | {}\n",
                        std::chrono::system_clock::now(),
                        config::toStr(LogLevel), std::this_thread::get_id(),
                        loc.file_name(), loc.function_name(), loc.line(), str);

    file.close();
  }

  template <config::LogVerbosity LogLevel>
  static void writeToLogNCl(
      std::string_view str,
      std::source_location loc = std::source_location::current())
  {
    std::cout << str << '\n';
    writeToLog<LogLevel>(str, loc);
  }
  Logger() = default;
  ~Logger() = default;
  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger& operator=(Logger&&) = delete;

 private:
  inline static std::string log_file;
};

namespace logger_presets {

void userInputError(std::string_view input_str, char symbol,
                    std::source_location loc = std::source_location::current());
void parsingInputError(
    std::string_view input_str, char symbol,
    std::source_location loc = std::source_location::current());
void defaultError(std::string_view metadata,
                  std::source_location loc = std::source_location::current());

template <typename T>
inline void acquiringResourceError(
    std::string_view metadata,
    std::source_location loc = std::source_location::current())
{
  auto ptr = acquireName<T>();

  Logger::writeToLogNCl<config::LogVerbosity::Error>(
      std::format("Error during acquiring {} with metadata {}", ptr.get(),
                  metadata),
      loc);
}
template <typename T>
inline void containerRemove(
    std::string_view metadata,
    std::source_location loc = std::source_location::current())
{
  auto ptr = acquireName<T>();

  Logger::writeToLog<config::LogVerbosity::Info>(
      std::format("Removed object from {} - metadata {}", ptr.get(), metadata),
      loc);
}
template <typename T>
inline void containerPush(
    std::string_view metadata,
    std::source_location loc = std::source_location::current())
{
  auto ptr = acquireName<T>();

  Logger::writeToLog<config::LogVerbosity::Info>(
      std::format("Pushed object to {} - metadata {}", ptr.get(), metadata),
      loc);
}

template <typename T>
inline void createObject(
    std::source_location loc = std::source_location::current())
{
  auto ptr = acquireName<T>();

  Logger::writeToLog<config::LogVerbosity::Info>(
      std::format("Starting {} creation", ptr.get()), loc);
}

void createdStaticContainer(
    std::string_view description,
    std::source_location loc = std::source_location::current());
void functionCall(std::source_location loc = std::source_location::current());
void menuQuit(std::source_location loc = std::source_location::current());
void wrongInput(std::source_location loc = std::source_location::current());
void userInput(std::string_view input,
               std::source_location loc = std::source_location::current());
}  // namespace logger_presets

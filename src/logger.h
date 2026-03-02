#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include "config.h"  //!!!

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
      file << "Log is started\n\n";
      file.close();
    }
  }
  static void writeToLog(
      config::LogVerbosity log_level, std::string_view str,
      std::source_location loc = std::source_location::current())
  {
    if (log_level > config::kLogVerbosity) {
      return;
    }
    std::ofstream file(log_file, std::ios::app);
    if (!file.is_open()) {
      std::cerr << "Couldn't open the log_file!\n";
      return;
    }
    file << '[' << std::chrono::system_clock::now()
         << "]: " << config::toStr(log_level) << '\n'
         << loc.file_name() << ' ' << loc.function_name() << ' ' << loc.line()
         << '\n'
         << str << "\n\n";

    file.close();
  }
  static void writeToLogNCl(
      const config::LogVerbosity log_level, std::string_view str,
      std::source_location loc = std::source_location::current())
  {
    std::cout << str << '\n';
    writeToLog(log_level, str, loc);
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

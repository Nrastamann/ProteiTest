#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <source_location>
#include <string>
#include <string_view>
#include "config.h"  //!!!

class Logger {

 public:
  void loggerInit(const std::string& path = "logs")
  {
    _log_file = path;
    if (!std::filesystem::exists(_log_file)) {
      std::filesystem::create_directory(_log_file);
    }

    _log_file += "/log-";
    _log_file += __DATE__;
    _log_file += ' ';
    _log_file += __TIME__;
    std::ofstream file(_log_file);
    if (file.is_open()) {
      file.close();
    }
  }
  void writeToLog(config::LogVerbosity log_level, std::string_view str,
                  std::source_location loc = std::source_location::current())
  {
    if (log_level > config::kLogVerbosity) {
      return;
    }
    std::ofstream file(_log_file, std::ios::app);
    if (!file.is_open()) {
      std::cerr << "Couldn't open the log_file!\n";
      return;
    }
    file << std::setprecision(3);
    file << __DATE__ << ' ' << std::chrono::system_clock::now() << ": "
         << +static_cast<uint8_t>(log_level) << '\n'
         << str << '\n'
         << loc.file_name() << ' ' << loc.function_name() << ' ' << loc.line()
         << "\n\n";

    file.close();
    file.flush();
  }
  void writeToLogNCl(config::LogVerbosity log_level, std::string_view str,
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
  std::string _log_file;
};

#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "config.hpp"
#include "ip_addr.hpp"
#include "logger.hpp"

namespace hashed {
inline size_t const kAddr = {std::hash<std::string_view>{}("-a")};
inline size_t const kPort = {std::hash<std::string_view>{}("-p")};
inline size_t const kIMSI = {std::hash<std::string_view>{}("-i")};
inline size_t const kIMEI = {std::hash<std::string_view>{}("-e")};
inline size_t const kMSISDN = {std::hash<std::string_view>{}("-m")};
inline size_t const kHelp = {std::hash<std::string_view>{}("-h")};
inline size_t const kVerbosity = {std::hash<std::string_view>{}("-v")};
inline size_t const kEPCPath = {std::hash<std::string_view>{}("-c")};
inline size_t const kENodeBPath = {std::hash<std::string_view>{}("-n")};
inline size_t const kXPos = {std::hash<std::string_view>{}("-x")};
[[maybe_unused]] inline size_t const kYPos = {std::hash<std::string_view>{}("-y")};
}  // namespace hashed

namespace parsing {
std::string composeNumber(std::string_view index_str);

enum class ParseResult : uint8_t { NO_ERR, WRONG_FLAG, NO_ARGUMENT, HELP, SV_PARSING_ERR };
class ArgHolder {
  template <typename T>
  using container = std::vector<T>;

 public:
  using argsMap = std::unordered_map<size_t, std::function<bool(std::string&, ArgHolder&)>>;
  using parseToken = std::string;
  using argument_type = parseToken;

  bool setArgument(std::size_t hash, std::string& value, ArgHolder::argsMap& map)
  {
    logging::SingleThreadPresets::functionCall();

    auto it = map.find(hash);
    bool return_value = it != map.end();

    return_value = return_value ? it->second(value, *this) : false;

    return return_value;
  }  // namespace parsing

  bool pushAddr(std::string token);
  bool pushMSISDN(std::string token) { return pushValue(std::move(token), _msisdn); }
  bool pushIMSI(std::string token) { return pushValue(std::move(token), _imsi); }
  bool pushIMEI(std::string token) { return pushValue(std::move(token), _imei); }
  bool pushX(std::string token) { return pushValue(std::move(token), _x); }
  [[maybe_unused]] bool pushY(std::string token) { return pushValue(std::move(token), _y); }
  [[maybe_unused]] bool pushEPC(std::string token)
  {
    _epc_path = std::move(token);
    return true;
  }
  [[maybe_unused]] bool pushENodeB(std::string token)
  {
    _enodeb_path = std::move(token);
    return true;
  }

  template <logging::LoggerPolicy T>
  bool setLog(std::string token, T)
  {
    std::ranges::transform(token, token.begin(), ::tolower);

    config::LogVerbosity log_level = config::fromStr(token);

    switch (log_level) {
      case config::LogVerbosity::WRONG_FLAG:
        return false;
      default:
        logging::SingleThreadLogger::verbosity_logger = {log_level};
        logging::MultithreadLogger::verbosity_logger = {log_level};
    }
    return true;
  }

  container<network_addr::IpAddr>&& getAddr() { return std::move(_addresses); }

  std::string getEPCPath() { return std::move(_epc_path); }
  std::string getENodeBPath() { return std::move(_enodeb_path); }

  [[nodiscard]] uint64_t getIMEI() const { return _imei; }
  [[nodiscard]] uint64_t getMSISDN() const { return _msisdn; }
  [[nodiscard]] uint64_t getIMSI() const { return _imsi; }

 private:
  template <typename T>
  bool pushValue(std::string&& token, T& value)
  {
    logging::SingleThreadPresets::functionCall();

    auto parsed_addr = parseNumber<T>(composeNumber(std::move(token)));

    if (!parsed_addr.has_value()) {
      logging::SingleThreadPresets::defaultError(
          std::format("Couldn't collect number - {}", token));
      return false;
    }

    value = parsed_addr.value();
    return true;
  }

  template <typename T>
  std::expected<T, ParseResult> parseNumber(std::string_view index)
  {
    logging::SingleThreadPresets::functionCall();

    T index_number{};
    auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);
    if (ec != std::errc() || ptr != index.end() || index.size() == 0) {
      logging::SingleThreadPresets::userInputError(index, *ptr);
      return std::unexpected(ParseResult::SV_PARSING_ERR);
    }

    return index_number;
  }

  container<network_addr::IpAddr> _addresses;

  std::string _epc_path;
  std::string _enodeb_path;

  uint64_t _imei;
  uint64_t _msisdn;
  uint64_t _imsi;
  uint64_t _port;

  int64_t _x;
  [[maybe_unused]] int64_t _y;
};

inline bool isNumericFlag(size_t hash)
{
  return hash == hashed::kAddr || hash == hashed::kIMSI || hash == hashed::kIMEI ||
         hash == hashed::kMSISDN || hash == hashed::kXPos || hash == hashed::kYPos;
}

std::expected<ArgHolder, ParseResult> parseArguments(int argc, char** argv,
                                                     ArgHolder::argsMap& flags);
inline static parsing::ArgHolder::argsMap& getArgSetterMain()
{
  static parsing::ArgHolder::argsMap map = {
      {hashed::kAddr,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushAddr(std::move(value));
       }},
      {hashed::kHelp, [](std::string&, parsing::ArgHolder&) { return true; }},
      {hashed::kVerbosity,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.setLog(std::move(value), logging::SingleThreadPolicy{});
       }},
      {hashed::kXPos,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushX(std::move(value));
       }},
      {hashed::kYPos,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushY(std::move(value));
       }},
      {hashed::kIMEI,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushIMEI(std::move(value));
       }},
      {hashed::kIMSI,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushIMSI(std::move(value));
       }},
      {hashed::kMSISDN,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushMSISDN(std::move(value));
       }},
  };

  return map;
}
};  // namespace parsing

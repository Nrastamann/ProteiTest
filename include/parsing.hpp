#pragma once
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "custom_types.hpp"
#include "ip_addr.hpp"
#include "nlohmann/json_fwd.hpp"
#include "settings.hpp"

namespace hashed {
inline size_t const kAddrHash = {std::hash<std::string_view>{}("-a")};
inline size_t const kAddrBigHash = {std::hash<std::string_view>{}("-A")};
inline size_t const kPortHash = {std::hash<std::string_view>{}("-p")};
inline size_t const kRoleHash = {std::hash<std::string_view>{}("-r")};
inline size_t const kIndexHash = {std::hash<std::string_view>{}("-i")};
inline size_t const kLibHash = {std::hash<std::string_view>{}("-l")};
inline size_t const kHelp = {std::hash<std::string_view>{}("-h")};
}  // namespace hashed

namespace parsing {
enum class ParseResult : uint8_t { NO_ERR, WRONG_FLAG, NO_ARGUMENT, HELP, SV_PARSING_ERR };
class ArgHolder {
  template <typename T>
  using container = std::vector<T>;

 public:
  using parseToken = std::string;
  using argument_type = parseToken;

  bool pushAddr(std::string&& token);

  bool setArgument(std::size_t hash, std::string& value)
  {
    logging::SingleThreadPresets::functionCall();

    auto& map = getArgSetter();

    auto it = map.find(hash);
    bool return_value = it != map.end();

    return_value = return_value ? it->second(value) : false;

    return return_value;
  }  // namespace parsing

  container<network_addr::IpAddr>&& getAddr() { return std::move(_addresses); }
  container<std::string> getLibs() { return std::move(_libs); }
  std::string getRole() { return std::move(_role); }
  [[nodiscard]] size_t getIndex() const { return _index; }

 private:
  std::unordered_map<size_t, std::function<bool(std::string&)>>& getArgSetter()
  {
    static std::unordered_map<size_t, std::function<bool(std::string&)>> map = {
        {hashed::kAddrHash,
         [this](std::string& value) { return this->pushAddr(std::move(value)); }},
        {hashed::kAddrBigHash,
         [this](std::string& value) { return this->pushAddr(std::move(value)); }},
        {hashed::kIndexHash,
         [this](std::string& value) { return this->parseIndex(std::move(value)); }},
        {hashed::kRoleHash,
         [this](std::string& value) {
           _role = std::move(value);
           return true;
         }},
        {hashed::kLibHash, [this](std::string& value) {
           _libs.emplace_back(std::move(value));
           return true;
         }}};
    return map;
  }

  bool parseIndex(std::string_view index);

  container<network_addr::IpAddr> _addresses;
  container<std::string> _libs;
  std::string _role = "User";
  size_t _index{};
};

std::expected<ArgHolder, ParseResult> parseArguments(int argc, char** argv);

custom_types::PolymorphicVectorQuad parseStringVector(nlohmann::json& json);

[[nodiscard]] std::expected<AppSettings, bool> createSettings(
    std::vector<std::string> wrapped_input, std::string_view help_text);
};  // namespace parsing

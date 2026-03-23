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
#include "logger.hpp"
#include "nlohmann/json_fwd.hpp"

namespace hashed {
inline size_t const kAddrHash = {std::hash<std::string_view>{}("-a")};
inline size_t const kRoleHash = {std::hash<std::string_view>{}("-r")};
inline size_t const kIndexHash = {std::hash<std::string_view>{}("-i")};
inline size_t const kLibHash = {std::hash<std::string_view>{}("-l")};
inline size_t const kHelp = {std::hash<std::string_view>{}("-h")};
inline size_t const kPort = {std::hash<std::string_view>{}("-p")};
}  // namespace hashed

namespace parsing {
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

  bool pushIndex(std::string token);
  bool pushAddr(std::string token);
  //move into
  bool pushRole(std::string token)
  {
    _role = token;
    return true;
  }
  //move into
  bool pushLib(std::string token)
  {
    _libs.emplace_back(token);
    return true;
  }

  container<network_addr::IpAddr>&& getAddr() { return std::move(_addresses); }
  container<std::string> getLibs() { return std::move(_libs); }
  std::string getRole() { return std::move(_role); }
  [[nodiscard]] size_t getIndex() const { return _index; }

 private:
  container<network_addr::IpAddr> _addresses;
  container<std::string> _libs;
  std::string _role = "User";
  size_t _index{};
};

inline bool isNumericFlag(size_t hash)
{
  return hash == hashed::kAddrHash || hash == hashed::kIndexHash;
}

std::expected<ArgHolder, ParseResult> parseArguments(int argc, char** argv,
                                                     ArgHolder::argsMap& flags);

custom_types::PolymorphicVectorQuad parseStringVector(nlohmann::json& json);

inline static parsing::ArgHolder::argsMap& getArgSetterMain()
{
  static parsing::ArgHolder::argsMap map = {
      {hashed::kAddrHash,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushAddr(std::move(value));
       }},
      {hashed::kIndexHash,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushIndex(std::move(value));
       }},
      {hashed::kRoleHash,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushRole(std::move(value));
       }},
      {hashed::kLibHash,
       [](std::string& value, parsing::ArgHolder& holder) {
         return holder.pushLib(std::move(value));
       }},
      {hashed::kHelp, [](std::string&, parsing::ArgHolder&) { return true; }},

  };

  return map;
}
};  // namespace parsing

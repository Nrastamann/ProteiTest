#include <algorithm>
#include <charconv>
#include <string>
#include <unordered_map>
#include <variant>

#include <nlohmann/json.hpp>

#include "data_pool.hpp"
#include "display.hpp"
#include "nlohmann/json_fwd.hpp"
#include "parsing.hpp"

#include <sys/socket.h>
#include "custom_types.hpp"
#include "ip_addr.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"
#include "resources_test.hpp"

namespace custom_types {
const std::unordered_map<EnumTypes, any_type>& getDefaultValues()
{
  static std::unordered_map<EnumTypes, any_type> type_dispatch{
      {EnumTypes::Bool, false},
      {EnumTypes::Char, char{}},
      {EnumTypes::Double, 0.},
      {EnumTypes::Float, 0.F},
      {EnumTypes::Int, 0},
      {EnumTypes::Int16, static_cast<int16_t>(0)},
      {EnumTypes::Int32, static_cast<int32_t>(0)},
      {EnumTypes::Int64, static_cast<int64_t>(0)},
      {EnumTypes::Int8, static_cast<int8_t>(0)},
      {EnumTypes::UInt16, static_cast<uint16_t>(0)},
      {EnumTypes::UInt32, static_cast<uint32_t>(0)},
      {EnumTypes::UInt64, static_cast<uint64_t>(0)},
      {EnumTypes::UInt8, static_cast<uint8_t>(0)},
      {EnumTypes::String, ""},

  };

  logging::logger_presets::createdStaticContainer("EnumTypes - default_value - unordered_map");
  return type_dispatch;
}
}  // namespace custom_types

namespace menu_functions {
static constexpr size_t kMaxBuffer{4096};

template <isPartOf T>
static inline std::from_chars_result convertAnyType(std::string_view string_input,
                                                    T& emplace_element)
{
  logging::logger_presets::functionCall();

  std::from_chars_result conv_result{};

  T result{};
  conv_result = std::from_chars(string_input.begin(), string_input.end(), result);

  emplace_element = result;
  return conv_result;
}

template <>
inline std::from_chars_result convertAnyType<std::string>(std::string_view string_input,
                                                          std::string& emplace_element)
{
  logging::logger_presets::functionCall();

  emplace_element = string_input.data();
  return {.ptr = string_input.end(), .ec = std::errc()};
}

//need to support 'true'/'false' input
static inline std::from_chars_result convertAnyTypeBool(std::string_view string_input,
                                                        bool& emplace_element,
                                                        size_t hashed_input)
{
  logging::logger_presets::functionCall();

  std::from_chars_result conv_result{.ptr = string_input.end(), .ec = std::errc()};

  bool result = hashed_input == hashed::kTrueSymbolic;

  if (!result && hashed_input != hashed::kFalseSymbolic) {
    size_t input{};
    conv_result = std::from_chars(string_input.begin(), string_input.end(), input);
    result = input == 1;
  }

  emplace_element = result;
  return conv_result;
}

static nlohmann::json getJson(data_storage::PolymorphicDimensionalVector& vector)
{
  logging::logger_presets::functionCall();

  nlohmann::json json_to_send;

  std::string vector_str;
  for (const auto& i : vector._vec) {
    std::visit(
        custom_types::Visitor{
            [&vector_str](auto const& variant_val) {
              vector_str += std::format("{} ", variant_val);
            },
        },
        i);
  }

  json_to_send["TypeHash"] = vector._type_hash;
  json_to_send["Vector"] = std::move(vector_str);

  return json_to_send;
}

static bool sendToSocket(const network_addr::IpAddr& ip_addr, std::string_view str_to_send,
                         std::string& str_to_get, nlohmann::json& json_to_send,
                         data_storage::DataPool& datapool)
{
  logging::logger_presets::functionCall();

  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    logging::logger_presets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't create socket to {}", ip_addr));
    return false;
  }

  sockaddr_in server_addr{.sin_family = AF_INET,
                          .sin_port = htons(ip_addr._port),
                          .sin_addr{ip_addr.addrToNetwork()},
                          .sin_zero{0}};
  //NOLINTNEXTLINE
  int res =
      connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

  if (res != 0) {
    close(client_socket);
    logging::logger_presets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't connect to {}", ip_addr));
    return false;
  }

  if (send(client_socket, str_to_send.data(), str_to_send.length(), 0) == -1) {
    close(client_socket);
    logging::logger_presets::defaultError(
        std::format("Couldn't send data to socket {}", client_socket));
    return false;
  }

  std::ranges::fill(str_to_get, 0);

  if (recv(client_socket, str_to_get.data(), str_to_get.length(), 0) == -1) {
    logging::logger_presets::defaultError(
        std::format("Couldn't get data from socket {}", client_socket));
  }

  json_to_send = nlohmann::json::parse(str_to_get);
  datapool.push(data_storage::PolymorphicDimensionalVector{
      parsing::parseStringVector(json_to_send), json_to_send["TypeHash"]});

  close(client_socket);
  return true;
}

std::from_chars_result emplaceInVector(
    custom_types::any_type& emplace_element, std::string_view string_input,
    size_t hashed_input)  //with hashed_input to support 'true'/'false' insert
{
  logging::logger_presets::functionCall();

  std::from_chars_result conv_result{};

  conv_result = std::visit(
      custom_types::Visitor{[string_input]<typename T>(T& emplace_element) {
                              return convertAnyType<T>(string_input, emplace_element);
                            },
                            [string_input, hashed_input](bool& emplace_element) {
                              return convertAnyTypeBool(string_input, emplace_element,
                                                        hashed_input);
                            }},
      emplace_element);

  return conv_result;
}

void changeType(AppSettings& settings)
{
  logging::logger_presets::functionCall();

  display::clearScreen();
  std::string string_input;
  const auto& implemented_types_reference = custom_types::getHashToTypeInfo();

  while (true) {
    std::cout << "Enter new type (name must correspond with c++ types) or "
                 "enter 'quit' if you've changed your "
                 "mind\nlist of supported type is:\n\t-all uint/int types with "
                 "bit width\n\t-float\n\t-double\n\t-string\n\t-bool\n\t-common "
                 "int\n\t-char\nEnter new type: ";

    std::cin >> string_input;
    logging::logger_presets::userInput(string_input);

    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    size_t hashed_input = std::hash<std::string_view>{}(string_input);

    if (hashed_input == hashed::kQuit) {
      logging::logger_presets::menuQuit();
      return;
    }

    if (std::cin.good() && implemented_types_reference.contains(hashed_input)) {
      settings.setTypeHash(hashed_input);
      settings.setTypeEnum(custom_types::getHashToTypeInfo().at(hashed_input).first);

      logging::logger_presets::menuQuit();
      return;
    }

    logging::logger_presets::wrongInput();
    display::clearCinBuffer();
  }
}

void changeName(AppSettings& settings)
{
  logging::logger_presets::functionCall();

  display::clearScreen();
  std::string string_input;
  while (true) {
    std::cout << "Enter your new name or\n"
                 "enter 'quit' if you've changed your mind: ";

    std::cin >> string_input;
    logging::logger_presets::userInput(string_input);
    std::string lowered_input;
    std::ranges::transform(string_input, lowered_input.begin(), ::tolower);
    if (std::hash<std::string_view>{}(lowered_input) == hashed::kQuit) {
      logging::logger_presets::menuQuit();
      return;
    }

    if (std::cin.good()) {
      settings.setName(std::move(string_input));
      logging::logger_presets::menuQuit();
      return;
    }

    display::clearCinBuffer();
    logging::logger_presets::wrongInput();
  }
}

void enterVector(data_storage::DataPool& vector, AppSettings const& settings)
{
  namespace rn = std::ranges;
  logging::logger_presets::functionCall();

  custom_types::PolymorphicVectorQuad spare_vector;
  std::cout << "Enter " << custom_types::kVectorDimensionsAmount << "-dimensional vector of "
            << custom_types::getHashToTypeInfo().at(settings.cgetTypeHash()).second
            << " or "
               "enter 'quit' if you've changed your "
               "mind.\nFormat is "
            << custom_types::kVectorDimensionsAmount << " values separated by whitespaces: ";

  bool is_conversion_not_done = true;
  std::string string_input;
  const auto& default_value = custom_types::getDefaultValues().at(settings.cgetTypeEnum());

  rn::fill(spare_vector, default_value);

  while (is_conversion_not_done) {
    is_conversion_not_done = false;

    for (auto& element : spare_vector) {
      std::cin >> string_input;
      logging::logger_presets::userInput(string_input);

      std::string lowercase_input = string_input;
      std::ranges::transform(lowercase_input, lowercase_input.begin(), ::tolower);
      size_t hashed_input = std::hash<std::string_view>{}(lowercase_input);

      if (hashed_input == hashed::kQuit) {
        logging::logger_presets::menuQuit();
        return;
      }

      auto [ptr, ec] = emplaceInVector(element, string_input, hashed_input);

      if (ec != std::errc() || ptr != string_input.end().base()) {
        is_conversion_not_done = true;
        logging::logger_presets::wrongInput();
        display::clearCinBuffer();
        break;
      }
    }
  }

  logging::logger_presets::menuQuit();
  vector.push(
      data_storage::PolymorphicDimensionalVector{spare_vector, settings.cgetTypeHash()});
}

void emptyQueue(data_storage::DataPool& data_pool, NonConstTag)
{

  logging::logger_presets::functionCall();

  while (data_pool.size() > 0) {
    auto vec = data_pool.front()._vec;
    for (const auto& i : vec) {
      std::visit(custom_types::Visitor{
                     [](auto const& variant_val) { std::cout << variant_val << ' '; },
                     [](int8_t value) { std::cout << +value << ' '; },
                     [](uint8_t value) { std::cout << +value << ' '; }},
                 i);
    }
    std::cout << " - "
              << custom_types::getHashToTypeInfo().at(data_pool.front()._type_hash).second
              << '\n';

    data_pool.pop();
  }

  logging::logger_presets::menuQuit();
  std::cout << "Queue is empty\n";
}

void printVector(data_storage::DataPool& arr, NonConstTag)
{
  logging::logger_presets::menuQuit();

  if (arr.size() == 0) {
    std::cout << "Empty queue\n";
    logging::Logger::writeToLog<config::LogVerbosity::Warning>(
        "Empty queue at printing, quiting procedure");
    return;
  }

  for (const auto& i : arr.front()._vec) {
    std::visit(
        custom_types::Visitor{[](auto const& variant_val) { std::cout << variant_val << ' '; },
                              [](int8_t value) { std::cout << +value << ' '; },
                              [](uint8_t value) { std::cout << +value << ' '; }},
        i);
  }
  std::cout << '\n';
  logging::logger_presets::menuQuit();
}
void sendToServer([[maybe_unused]] data_storage::DataPool& datapool,
                  const AppSettings& settings)
{

  logging::logger_presets::functionCall();
  if (datapool.size() == 0) {
    logging::logger_presets::defaultError("Empty datapool, can't send anything");
    return;
  }

  auto addresses = settings.cgetAddress();
  if (!resources_tests::ConnectionTest{addresses}()) {
    logging::logger_presets::acquiringResourceError<resources_tests::ConnectionTest>(
        "Couldn't access some of address to send vector");
    return;
  }

  nlohmann::json json_to_send = getJson(datapool.front());

  std::string str_to_send = json_to_send.dump();
  std::string str_to_get;

  logging::logger_presets::userInput(str_to_send);

  str_to_get.resize(kMaxBuffer);

  for (const auto& ip_addr : addresses) {
    sendToSocket(ip_addr, str_to_send, str_to_get, json_to_send, datapool);
  }
  datapool.pop();
}
}  // namespace menu_functions

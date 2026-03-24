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

namespace menu_functions {
static constexpr size_t kMaxBuffer{4096};
namespace {
template <typename T>
concept isPartOf = std::is_assignable_v<custom_types::any_type, T>;
}  // namespace

template <isPartOf T>
static inline std::from_chars_result convertAnyType(std::string_view string_input,
                                                    T& emplace_element)
{
  logging::SingleThreadPresets::functionCall();

  std::from_chars_result conv_result{};

  conv_result = std::from_chars(string_input.begin(), string_input.end(), emplace_element);

  return conv_result;
}

template <>
inline std::from_chars_result convertAnyType<std::string>(std::string_view string_input,
                                                          std::string& emplace_element)
{
  logging::SingleThreadPresets::functionCall();

  emplace_element = string_input.data();
  return {.ptr = string_input.end(), .ec = std::errc()};
}

//need to support 'true'/'false' input
static inline std::from_chars_result convertAnyTypeBool(std::string_view string_input,
                                                        bool& emplace_element,
                                                        size_t hashed_input)
{
  logging::SingleThreadPresets::functionCall();

  std::from_chars_result conv_result{.ptr = string_input.end(), .ec = std::errc()};

  emplace_element = hashed_input == hashed::kTrueSymbolic;

  if (!emplace_element && hashed_input != hashed::kFalseSymbolic) {
    size_t input{};
    conv_result = std::from_chars(string_input.begin(), string_input.end(), input);
    emplace_element = input == 1;
  }

  return conv_result;
}

static nlohmann::json getJson(data_storage::PolymorphicDimensionalVector& vector)
{
  logging::SingleThreadPresets::functionCall();

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

  json_to_send["TypeHash"] = vector.getHash();
  json_to_send["Vector"] = std::move(vector_str);

  return json_to_send;
}

static bool sendToSocket(const network_addr::IpAddr& ip_addr, std::string_view str_to_send,
                         std::string& str_to_get, nlohmann::json& json_to_send,
                         data_storage::DataPool& datapool)
{
  logging::SingleThreadPresets::functionCall();

  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket == -1) {
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
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
    logging::SingleThreadPresets::acquiringResourceError<resources_tests::ConnectionTest>(
        std::format("couldn't connect to {}", ip_addr));
    return false;
  }

  if (send(client_socket, str_to_send.data(), str_to_send.length(), 0) == -1) {
    close(client_socket);
    logging::SingleThreadPresets::defaultError(
        std::format("Couldn't send data to socket {}", client_socket));
    return false;
  }

  std::ranges::fill(str_to_get, 0);

  if (recv(client_socket, str_to_get.data(), str_to_get.length(), 0) == -1) {
    logging::SingleThreadPresets::defaultError(
        std::format("Couldn't get data from socket {}", client_socket));
  }

  json_to_send = nlohmann::json::parse(str_to_get);
  auto parse_result = parsing::parseStringVector(json_to_send);
  if (!parse_result.has_value()) {
    return false;
  }

  datapool.push(data_storage::PolymorphicDimensionalVector{std::move(parse_result.value())});

  close(client_socket);
  return true;
}

std::from_chars_result emplaceInVector(
    custom_types::any_type& emplace_element, std::string_view string_input,
    size_t hashed_input)  //with hashed_input to support 'true'/'false' insert
{
  logging::SingleThreadPresets::functionCall();

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
  logging::SingleThreadPresets::functionCall();

  display::clearScreen();
  std::string string_input;

  const auto& type_dispatch = custom_types::getDefaultValues();

  while (true) {
    std::cout << "Enter new type (name must correspond with c++ types) or "
                 "enter 'quit' if you've changed your "
                 "mind\nlist of supported type is:\n\t-all uint/int types with "
                 "bit width\n\t-float\n\t-double\n\t-string\n\t-bool\n\t-common "
                 "int\n\t-char\nEnter new type: ";

    std::cin >> string_input;
    logging::SingleThreadPresets::userInput(string_input);

    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    size_t hashed_input = std::hash<std::string_view>{}(string_input);

    if (hashed_input == hashed::kQuit) {
      logging::SingleThreadPresets::menuQuit();
      return;
    }

    if (std::cin.good() && type_dispatch.contains(hashed_input)) {
      settings.setTypeHash(hashed_input);

      logging::SingleThreadPresets::menuQuit();
      return;
    }

    logging::SingleThreadPresets::wrongInput();
    display::clearCinBuffer();
  }
}

void changeName(AppSettings& settings)
{
  logging::SingleThreadPresets::functionCall();

  display::clearScreen();
  std::string string_input;
  while (true) {
    std::cout << "Enter your new name or\n"
                 "enter 'quit' if you've changed your mind: ";

    std::cin >> string_input;
    logging::SingleThreadPresets::userInput(string_input);
    std::string lowered_input;
    std::ranges::transform(string_input, lowered_input.begin(), ::tolower);
    if (std::hash<std::string_view>{}(lowered_input) == hashed::kQuit) {
      logging::SingleThreadPresets::menuQuit();
      return;
    }

    if (std::cin.good()) {
      settings.setName(std::move(string_input));
      logging::SingleThreadPresets::menuQuit();
      return;
    }

    display::clearCinBuffer();
    logging::SingleThreadPresets::wrongInput();
  }
}

void enterVector(data_storage::DataPool& vector, AppSettings const& settings)
{
  namespace rn = std::ranges;
  logging::SingleThreadPresets::functionCall();
  auto& default_values = custom_types::getDefaultValues().at(settings.cgetTypeHash());
  custom_types::PolymorphicVectorQuad spare_vector;
  std::cout << "Enter " << custom_types::kVectorDimensionsAmount << "-dimensional vector of "
            << custom_types::getTypename(default_values)
            << " or "
               "enter 'quit' if you've changed your "
               "mind.\nFormat is "
            << custom_types::kVectorDimensionsAmount << " values separated by whitespaces: ";

  bool is_conversion_not_done = true;
  std::string string_input;
  const auto& default_value = custom_types::getDefaultValues().at(settings.cgetTypeHash());

  rn::fill(spare_vector, default_value);

  while (is_conversion_not_done) {
    is_conversion_not_done = false;

    for (auto& element : spare_vector) {
      std::cin >> string_input;
      logging::SingleThreadPresets::userInput(string_input);

      std::string lowercase_input = string_input;
      std::ranges::transform(lowercase_input, lowercase_input.begin(), ::tolower);
      size_t hashed_input = std::hash<std::string_view>{}(lowercase_input);

      if (hashed_input == hashed::kQuit) {
        logging::SingleThreadPresets::menuQuit();
        return;
      }

      auto [ptr, ec] = emplaceInVector(element, string_input, hashed_input);

      if (ec != std::errc() || ptr != string_input.end().base()) {
        is_conversion_not_done = true;
        logging::SingleThreadPresets::wrongInput();
        display::clearCinBuffer();
        break;
      }
    }
  }

  logging::SingleThreadPresets::menuQuit();
  vector.push(data_storage::PolymorphicDimensionalVector{spare_vector});
}

void emptyQueue(data_storage::DataPool& data_pool, NonConstTag)
{

  logging::SingleThreadPresets::functionCall();

  while (data_pool.size() > 0) {
    auto vec = data_pool.front();
    for (const auto& i : vec._vec) {
      std::visit(custom_types::Visitor{
                     [](auto const& variant_val) { std::cout << variant_val << ' '; },
                     [](int8_t value) { std::cout << +value << ' '; },
                     [](uint8_t value) { std::cout << +value << ' '; }},
                 i);
    }
    std::cout << " - " << vec.getTypename() << '\n';

    data_pool.pop();
  }

  logging::SingleThreadPresets::menuQuit();
  std::cout << "Queue is empty\n";
}

void printVector(data_storage::DataPool& arr, NonConstTag)
{
  logging::SingleThreadPresets::menuQuit();

  if (arr.size() == 0) {
    std::cout << "Empty queue\n";
    logging::SingleThreadLogger::writeToLog<config::LogVerbosity::Warning>(
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
  logging::SingleThreadPresets::menuQuit();
}

void sendToServer(data_storage::DataPool& datapool, const AppSettings& settings)
{

  logging::SingleThreadPresets::functionCall();
  if (datapool.size() == 0) {
    logging::SingleThreadPresets::defaultError("Empty datapool, can't send anything");
    return;
  }

  auto addresses = settings.cgetAddress();

  nlohmann::json json_to_send = getJson(datapool.front());

  std::string str_to_send = json_to_send.dump();
  std::string str_to_get;

  logging::SingleThreadPresets::userInput(str_to_send);

  str_to_get.resize(kMaxBuffer);

  for (const auto& ip_addr : addresses) {
    if (!sendToSocket(ip_addr, str_to_send, str_to_get, json_to_send, datapool)) {
      logging::SingleThreadPresets::defaultError(
          std::format("Couldn't send/process/recieve data at {}", ip_addr));
    }
  }
  datapool.pop();
}
}  // namespace menu_functions

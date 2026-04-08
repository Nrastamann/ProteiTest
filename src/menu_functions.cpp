#include <algorithm>
#include <string>

#include <nlohmann/json.hpp>

#include "data_pool.hpp"
#include "display.hpp"
#include "parsing.hpp"

#include <sys/socket.h>
#include "logger.hpp"
#include "menu_functions.hpp"
#include "settings.hpp"

namespace menu_functions {
void status(data_storage::DataPool& sms, AppSettings& settings, BothNonConstTag)
{
  auto& exchanger = settings.getExchanger();
  std::cout << "SMS Status:\n";
  sms.printAll(exchanger.getContext().msisdn());
  std::cout << "Connectivity status:\n";

  std::cout << "Power:\t\t" << exchanger.power() << '\n';
  std::cout << "EnodeB number:\t" << exchanger.enodeb() << '\n';

  std::cout << "Active:\t\t" << exchanger.inActive() << '\n';
  std::cout << "Attached:\t" << exchanger.attached() << '\n';
  std::cout << "X:\t\t" << exchanger.x() << '\n';
}

void moveX(AppSettings& settings)
{
  logging::SingleThreadPresets::functionCall();
  display::clearScreen();
  std::string string_input;
  while (true) {
    std::cout << "Enter distance to emulate movement\n"
                 "enter 'quit' if you've changed your mind: ";

    std::cin >> string_input;
    logging::SingleThreadPresets::userInput(string_input);
    std::string lowered_input;
    lowered_input.resize(string_input.size());
    std::ranges::transform(string_input, lowered_input.begin(), ::tolower);

    if (std::hash<std::string_view>{}(lowered_input) == hashed::kQuit) {
      logging::SingleThreadPresets::menuQuit();
      return;
    }

    if (std::cin.good()) {
      auto parse_res = parsing::parseNumber<int64_t>(lowered_input);

      if (parse_res.has_value()) {

        settings.getExchanger().moveX(parse_res.value());

        logging::SingleThreadPresets::menuQuit();
        return;
      }
    }

    display::clearCinBuffer();
    logging::SingleThreadPresets::wrongInput();
  }
}
}  // namespace menu_functions
//static constexpr size_t kMaxBuffer{4096};

/*static nlohmann::json getJson(data_storage::PolymorphicDimensionalVector& vector)
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
*/
/*
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
*/
/*
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
      //      settings.setName(std::move(string_input));
      logging::SingleThreadPresets::menuQuit();
      return;
    }

    display::clearCinBuffer();
    logging::SingleThreadPresets::wrongInput();
  }
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
}*/

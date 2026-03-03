#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <variant>

#include "data_pool.hpp"
#include "display.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"
#include "static_containers.hpp"
#include "utility.hpp"

namespace static_containers {
const std::unordered_map<static_containers::EnumTypes, any_type>&
getDefaultValues()
{
  using static_containers::EnumTypes;
  static std::unordered_map<static_containers::EnumTypes, any_type>
      type_dispatch{
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

  logger_presets::createdStaticContainer(
      "EnumTypes - default_value - unordered_map");
  return type_dispatch;
}
}  // namespace static_containers

namespace menu_functions_protei {
template <isPartOf T>
static inline std::from_chars_result convertAnyType(
    std::string_view string_input, T& emplace_element)
{
  logger_presets::functionCall();

  std::from_chars_result conv_result{};

  T result{};
  conv_result =
      std::from_chars(string_input.begin(), string_input.end(), result);

  emplace_element = result;
  return conv_result;
}

template <>
inline std::from_chars_result convertAnyType<std::string>(
    std::string_view string_input, std::string& emplace_element)
{
  logger_presets::functionCall();

  emplace_element = string_input.data();
  return {.ptr = string_input.end(), .ec = std::errc()};
}

//need to support 'true'/'false' input
static inline std::from_chars_result convertAnyTypeBool(
    std::string_view string_input, bool& emplace_element, size_t hashed_input)
{
  logger_presets::functionCall();

  std::from_chars_result conv_result(string_input.end());

  bool result = hashed_input == hashed::kTrueSymbolic;

  if (!result && hashed_input != hashed::kFalseSymbolic) {
    size_t input{};
    conv_result =
        std::from_chars(string_input.begin(), string_input.end(), input);
    result = input == 1;
  }

  emplace_element = result;
  return conv_result;
}

inline static std::from_chars_result emplaceInVector(
    any_type& emplace_element, std::string_view string_input,
    size_t hashed_input)  //with hashed_input to support 'true'/'false' insert
{
  logger_presets::functionCall();

  std::from_chars_result conv_result{};

  conv_result = std::visit(
      Visitor{[string_input]<typename T>(T& emplace_element) {
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
  logger_presets::functionCall();

  ui_protei::clearScreen();
  std::string string_input;
  const auto& implemented_types_reference =
      static_containers::getHashToTypeInfo();

  while (true) {
    std::cout
        << "Enter new type (name must correspond with c++ types) or "
           "enter 'quit' if you've changed your "
           "mind\nlist of supported type is:\n\t-all uint/int types with "
           "bit width\n\t-float\n\t-double\n\t-string\n\t-bool\n\t-common "
           "int\n\t-char\nEnter new type: ";

    std::cin >> string_input;
    logger_presets::userInput(string_input);

    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    size_t hashed_input = std::hash<std::string_view>{}(string_input);

    if (hashed_input == hashed::kQuit) {
      logger_presets::menuQuit();
      return;
    }

    if (std::cin.good() && implemented_types_reference.contains(hashed_input)) {
      settings.setTypeHash(hashed_input);
      settings.setTypeEnum(
          static_containers::getHashToTypeInfo().at(hashed_input).first);

      logger_presets::menuQuit();
      return;
    }

    logger_presets::wrongInput();
    ui_protei::clearCinBuffer();
  }
}

void changeName(AppSettings& settings)
{
  logger_presets::functionCall();

  ui_protei::clearScreen();
  std::string string_input;
  while (true) {
    std::cout << "Enter your new name or\n"
                 "enter 'quit' if you've changed your mind: ";

    std::cin >> string_input;
    logger_presets::userInput(string_input);

    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    if (std::hash<std::string_view>{}(string_input) == hashed::kQuit) {
      logger_presets::menuQuit();
      return;
    }

    if (std::cin.good()) {
      settings.setName(std::move(string_input));
      logger_presets::menuQuit();
      return;
    }

    ui_protei::clearCinBuffer();
    logger_presets::wrongInput();
  }
}

void enterVector(DataPool& vector, AppSettings const& settings)
{
  logger_presets::functionCall();

  ProteiVector spare_vector;
  std::cout << "Enter " << kVectorDimensionsAmount << "-dimensional vector of "
            << static_containers::getHashToTypeInfo()
                   .at(settings.cgetTypeHash())
                   .second
            << " or "
               "enter 'quit' if you've changed your "
               "mind.\nFormat is "
            << kVectorDimensionsAmount << " values separated by whitespaces: ";

  bool is_conversion_not_done = true;
  std::string string_input;
  std::string lowercase_input;
  const auto& default_value =
      static_containers::getDefaultValues().at(settings.cgetTypeEnum());

  for (auto& i : spare_vector) {
    i = default_value;
  }

  while (is_conversion_not_done) {
    is_conversion_not_done = false;

    for (auto& element : spare_vector) {
      std::cin >> string_input;
      logger_presets::userInput(string_input);

      lowercase_input = string_input;
      std::ranges::transform(lowercase_input, lowercase_input.begin(),
                             ::tolower);
      size_t hashed_input = std::hash<std::string_view>{}(lowercase_input);

      if (hashed_input == hashed::kQuit) {
        logger_presets::menuQuit();
        return;
      }

      auto [ptr, ec] = emplaceInVector(element, string_input, hashed_input);

      if (ec != std::errc() || ptr != string_input.end().base()) {
        is_conversion_not_done = true;
        logger_presets::wrongInput();
        ui_protei::clearCinBuffer();
        break;
      }
    }
  }

  logger_presets::menuQuit();
  vector.push(
      PolymorphicDimensionalVector{spare_vector, settings.cgetTypeHash()});
}

void emptyQueue(DataPool& data_pool, NonConstTag)
{
  logger_presets::functionCall();

  while (data_pool.size() > 0) {
    auto vec = data_pool.front()._vec;
    for (const auto& i : vec) {
      std::visit(Visitor{[](auto const& variant_val) {
                           std::cout << variant_val << ' ';
                         },
                         [](int8_t value) { std::cout << +value << ' '; },
                         [](uint8_t value) { std::cout << +value << ' '; }},
                 i);
    }
    std::cout << " - "
              << static_containers::getHashToTypeInfo()
                     .at(data_pool.front()._type_hash)
                     .second
              << '\n';

    data_pool.pop();
  }

  logger_presets::menuQuit();
  std::cout << "Queue is empty\n";
}
void printVector(DataPool& arr, NonConstTag)
{
  logger_presets::menuQuit();

  if (arr.size() == 0) {
    std::cout << "Empty queue\n";
    Logger::writeToLog<config::LogVerbosity::Warning>(
        "Empty queue at printing, quiting procedure");
    return;
  }

  for (const auto& i : arr.front()._vec) {
    std::visit(Visitor{[](auto const& variant_val) {
                         std::cout << variant_val << ' ';
                       },
                       [](int8_t value) { std::cout << +value << ' '; },
                       [](uint8_t value) { std::cout << +value << ' '; }},
               i);
  }
  std::cout << '\n';
  logger_presets::menuQuit();
}

}  // namespace menu_functions_protei

#include <algorithm>
#include <charconv>

#include "display.hpp"
#include "hashed_values.hpp"
#include "menu_functions.hpp"
#include "static_containers.hpp"

void changeType(Settings& settings)
{
  clearScreen();
  std::string string_input;
  const auto& implemented_types_reference =
      static_containers::getImplementedTypes();

  while (true) {
    std::cout
        << "Enter new type (name must correspond with c++ types) or "
           "enter 'quit' if you've changed your "
           "mind\nlist of supported type is:\n\t-all uint/int types with "
           "bit width\n\t-float\n\t-double\n\t-string\n\t-bool\n\t-common "
           "int\n\t-char\nEnter new type: ";

    std::cin >> string_input;
    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    size_t hashed_input = std::hash<std::string_view>{}(string_input);

    if (hashed_input == hashed::kQuitMenuHash) {
      return;
    }

    if (std::cin.good() && implemented_types_reference.contains(hashed_input)) {
      settings.setTypeHash(hashed_input);
      settings.setTypeEnum(static_containers::getEnumType().at(hashed_input));
      return;
    }

    clearCinBuffer();
    std::cerr << "Wrong input, try again: ";
  }
}

void changeRole(Settings& settings)
{
  clearScreen();
  std::string string_input;
  std::size_t index{};
  while (true) {
    std::cout
        << "Enter your new role and index(first role, and then index) or\n"
           "enter 'quit' if you've changed your mind: ";

    std::cin >> string_input;
    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    if (std::hash<std::string_view>{}(string_input) == hashed::kQuitMenuHash) {
      return;
    }

    std::cin >> index;
    if (std::cin.good()) {
      settings.setRole(string_input);
      settings.setIndex(index);
      return;
    }

    clearCinBuffer();
    std::cerr << "Wrong input, try again: ";
  }
}

template <typename T, static_containers::EnumTypes EnumType>
static inline std::from_chars_result convertAnyType(
    std::string_view string_input, any_type& emplace_element)
{
  std::from_chars_result conv_result{};

  T result{};
  conv_result =
      std::from_chars(string_input.begin(), string_input.end(), result);

  emplace_element.emplace<static_cast<size_t>(EnumType)>(result);
  return conv_result;
}
template <>
inline std::from_chars_result
convertAnyType<std::string, static_containers::EnumTypes::String>(
    std::string_view string_input, any_type& emplace_element)
{
  emplace_element
      .emplace<static_cast<size_t>(static_containers::EnumTypes::String)>(
          string_input);
  return {.ptr = string_input.end(), .ec = std::errc()};
}

//need to support 'true'/'false' input
static inline std::from_chars_result convertAnyTypeBool(
    std::string_view string_input, any_type& emplace_element,
    size_t hashed_input)
{
  std::from_chars_result conv_result(string_input.end());

  bool result = hashed_input == hashed::kTrueHashSymbolic;

  if (!result && hashed_input != hashed::kFalseHashSymbolic) {
    size_t input{};
    conv_result =
        std::from_chars(string_input.begin(), string_input.end(), input);
    result = input == 1;
  }

  emplace_element
      .emplace<static_cast<size_t>(static_containers::EnumTypes::Bool)>(result);
  return conv_result;
}

inline static std::from_chars_result emplaceInVector(
    static_containers::EnumTypes current_type, any_type& emplace_element,
    std::string_view string_input,
    size_t hashed_input)  //with hashed_input to support 'true'/'false' insert
{
  std::from_chars_result conv_result{};

  switch (current_type) {
    case static_containers::EnumTypes::Int:
      conv_result = convertAnyType<int, static_containers::EnumTypes::Int>(
          string_input, emplace_element);
      break;

    case static_containers::EnumTypes::Float:
      conv_result = convertAnyType<float, static_containers::EnumTypes::Float>(
          string_input, emplace_element);
      break;
    case static_containers::EnumTypes::Double:
      conv_result =
          convertAnyType<double, static_containers::EnumTypes::Double>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::Char:
      conv_result = convertAnyType<char, static_containers::EnumTypes::Char>(
          string_input, emplace_element);
      break;
    case static_containers::EnumTypes::String:
      conv_result =
          convertAnyType<std::string, static_containers::EnumTypes::String>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::Bool: {
      conv_result =
          convertAnyTypeBool(string_input, emplace_element, hashed_input);
      break;
    }
    case static_containers::EnumTypes::Int8:
      conv_result = convertAnyType<int8_t, static_containers::EnumTypes::Int8>(
          string_input, emplace_element);
      break;
    case static_containers::EnumTypes::Int16:
      conv_result =
          convertAnyType<int16_t, static_containers::EnumTypes::Int16>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::Int32:
      conv_result =
          convertAnyType<int32_t, static_containers::EnumTypes::Int32>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::Int64:
      conv_result =
          convertAnyType<int64_t, static_containers::EnumTypes::Int64>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::UInt8:
      conv_result =
          convertAnyType<uint8_t, static_containers::EnumTypes::UInt8>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::UInt16:
      conv_result =
          convertAnyType<uint16_t, static_containers::EnumTypes::UInt16>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::UInt32:
      conv_result =
          convertAnyType<uint32_t, static_containers::EnumTypes::UInt32>(
              string_input, emplace_element);
      break;
    case static_containers::EnumTypes::UInt64:
      conv_result =
          convertAnyType<uint64_t, static_containers::EnumTypes::UInt64>(
              string_input, emplace_element);
      break;
  }

  return conv_result;
}

void enterVector(PolymorpicVector<kVectorDimensionsAmount>& vector,
                 Settings const& settings)
{
  PolymorpicVector<kVectorDimensionsAmount> spare_vector;

  std::cout << "Enter " << kVectorDimensionsAmount << "-dimensional vector of "
            << static_containers::getImplementedTypes().at(
                   settings.cgetTypeHash())
            << " or "
               "enter 'quit' if you've changed your "
               "mind.\nFormat is "
            << kVectorDimensionsAmount << " values separated by whitespaces: ";

  bool is_conversion_not_done = true;

  std::string string_input;
  std::string lowercase_input;

  while (is_conversion_not_done) {
    is_conversion_not_done = false;

    for (auto& element : spare_vector) {
      std::cin >> string_input;

      lowercase_input = string_input;
      std::ranges::transform(lowercase_input, lowercase_input.begin(),
                             ::tolower);
      size_t hashed_input = std::hash<std::string_view>{}(lowercase_input);

      if (hashed_input == hashed::kQuitMenuHash) {
        return;
      }

      static_containers::EnumTypes const current_type = settings.cgetTypeEnum();
      auto [ptr, ec] =
          emplaceInVector(current_type, element, string_input, hashed_input);

      if (ec != std::errc() || ptr != string_input.end().base()) {
        is_conversion_not_done = true;

        clearCinBuffer();
        std::cout << "Error during type conversion, re-input: ";
        break;
      }
    }
  }

  vector = spare_vector;
}

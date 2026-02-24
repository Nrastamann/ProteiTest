#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <functional>
#include <iostream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <variant>

#include "display.hpp"

constexpr size_t kIpAddrOctetAmount{4};
//constexpr size_t kIpAddrOctetSize{3};
constexpr size_t kVectorDimensionsAmount{4};

size_t const kAddrHash = {std::hash<std::string_view>{}("-a")};
size_t const kPortHash = {std::hash<std::string_view>{}("-p")};
size_t const kRoleHash = {std::hash<std::string_view>{}("-r")};
size_t const kIndexHash = {std::hash<std::string_view>{}("-i")};
size_t const kLibHash = {std::hash<std::string_view>{}("-L")};

size_t const kRoleMenuHash = std::hash<std::string_view>{}("role");
size_t const kTypeMenuHash = std::hash<std::string_view>{}("type");
size_t const kVectorMenuHash = std::hash<std::string_view>{}("vector");
size_t const kPrintHash = std::hash<std::string_view>{}("print");
size_t const kQuitMenuHash = std::hash<std::string_view>{}("quit");
size_t const kSettingsMenuHash = std::hash<std::string_view>{}("settings");

using any_type =
    std::variant<int, float, double, char, std::string, bool, int8_t, int16_t,
                 int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t>;

template <size_t N>
using PolymorpicVector = std::array<any_type, N>;

size_t const kIntHash = std::hash<std::string_view>{}("int");
size_t const kFloatHash = std::hash<std::string_view>{}("float");
size_t const kDoubleHash = std::hash<std::string_view>{}("double");
size_t const kCharHash = std::hash<std::string_view>{}("char");
size_t const kStringHash = std::hash<std::string_view>{}("string");
size_t const kBoolHash = std::hash<std::string_view>{}("bool");

size_t const kInt8Hash = std::hash<std::string_view>{}("int8_t");
size_t const kInt16Hash = std::hash<std::string_view>{}("int16_t");
size_t const kInt32Hash = std::hash<std::string_view>{}("int32_t");
size_t const kInt64Hash = std::hash<std::string_view>{}("int64_t");

size_t const kUInt8Hash = std::hash<std::string_view>{}("uint8_t");
size_t const kUInt16Hash = std::hash<std::string_view>{}("uint16_t");
size_t const kUInt32Hash = std::hash<std::string_view>{}("uint32_t");
size_t const kUInt64Hash = std::hash<std::string_view>{}("uint64_t");

size_t const kTrueHashSymbolic = std::hash<std::string_view>{}("true");
size_t const kFalseHashSymbolic = std::hash<std::string_view>{}("false");

enum class EnumTypes : uint8_t {
  Int,
  Float,
  Double,
  Char,
  String,
  Bool,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
};

enum class MenuOptions : uint8_t {
  ChangeRole,
  ChangeType,
  PrintSettings,
  EnterVector,
  QuitProgram,
  WrongOption,
  PrintCurrentVector
};

static std::unordered_map<size_t, std::string_view> const& getImplementedTypes()
{
  static std::unordered_map<size_t, std::string_view> const k_type_checker{
      {kIntHash, "int"},         {kFloatHash, "float"},
      {kDoubleHash, "double"},   {kCharHash, "char"},
      {kStringHash, "string"},   {kBoolHash, "bool"},

      {kInt8Hash, "int8_t"},     {kInt16Hash, "int16_t"},
      {kInt32Hash, "int32_t"},   {kInt64Hash, "int64_t"},

      {kUInt8Hash, "uint8_t"},   {kUInt16Hash, "uint16_t"},
      {kUInt32Hash, "uint32_t"}, {kUInt64Hash, "uint64_t"},
  };

  return k_type_checker;
}

static std::unordered_map<size_t, EnumTypes> const& getEnumType()
{
  static std::unordered_map<size_t, EnumTypes> const enum_types_converter{
      {kIntHash, EnumTypes::Int},       {kFloatHash, EnumTypes::Float},
      {kDoubleHash, EnumTypes::Double}, {kCharHash, EnumTypes::Char},
      {kStringHash, EnumTypes::String}, {kBoolHash, EnumTypes::Bool},

      {kInt8Hash, EnumTypes::Int8},     {kInt16Hash, EnumTypes::Int16},
      {kInt32Hash, EnumTypes::Int32},   {kInt64Hash, EnumTypes::Int64},

      {kUInt8Hash, EnumTypes::UInt8},   {kUInt16Hash, EnumTypes::UInt16},
      {kUInt32Hash, EnumTypes::UInt32}, {kUInt64Hash, EnumTypes::UInt64},
  };
  return enum_types_converter;
}

static std::unordered_map<size_t, MenuOptions> const& getMenuOptions()
{
  static std::unordered_map<size_t, MenuOptions> const k_menu_options{
      {kRoleMenuHash, MenuOptions::ChangeRole},
      {kTypeMenuHash, MenuOptions::ChangeType},
      {kVectorMenuHash, MenuOptions::EnterVector},
      {kPrintHash, MenuOptions::PrintCurrentVector},
      {kQuitMenuHash, MenuOptions::QuitProgram},
      {kSettingsMenuHash, MenuOptions::PrintSettings}};

  return k_menu_options;
}

class Settings {
  std::string _lib_name;
  std::string _role;
  std::array<uint8_t, kIpAddrOctetAmount> _ip_addr{};
  size_t _type_hash{kIntHash};
  EnumTypes _type_enum{EnumTypes::Int};
  size_t _port{};
  size_t _index{};

 public:
  [[nodiscard]] std::array<uint8_t, kIpAddrOctetAmount> const& getAddr() const
  {
    return _ip_addr;
  }

  [[nodiscard]] std::string const& cGetLibName() const { return _lib_name; }
  [[nodiscard]] size_t cgetTypeHash() const { return _type_hash; }
  [[nodiscard]] EnumTypes cgetTypeEnum() const { return _type_enum; }
  [[nodiscard]] std::string const& cgetRole() const { return _role; }
  [[nodiscard]] size_t cgetPort() const { return _port; }
  [[nodiscard]] size_t cgetIndex() const { return _index; }
  void setAddr(std::array<uint8_t, kIpAddrOctetAmount> const& ip_addr)
  {
    _ip_addr = ip_addr;
  }
  void setTypeHash(size_t hash) { _type_hash = hash; }
  void setTypeEnum(EnumTypes type) { _type_enum = type; }
  void setLibName(std::string_view lib_name) { _lib_name = lib_name; }
  void setRole(std::string_view role) { _role = role; }
  void setPort(size_t port) { _port = port; }
  void setIndex(size_t index) { _index = index; }
  friend std::ostream& operator<<(std::ostream& out, Settings const& settings);

  ~Settings() = default;
  Settings() = default;
  Settings(Settings const&) = default;
  Settings(Settings&&) = default;
  Settings& operator=(Settings const&) = default;
  Settings& operator=(Settings&&) = default;
};

std::ostream& operator<<(std::ostream& out, Settings const& settings)
{
  out << "========================\n";

  out << "Current settings:\n";

  out << "Role:\t\t" << settings._role << '\n';
  out << "Index:\t\t" << settings._index << '\n';
  out << "IP address:\t";
  size_t delimeter_index = 0;

  for (const auto& octet : settings._ip_addr) {
    char delimeter = ++delimeter_index < settings._ip_addr.size() ? '.' : ' ';
    out << +octet << delimeter;
  }

  out << '\n';
  out << "Port:\t\t" << settings._port << '\n';
  out << "Library name:\t" << settings._lib_name << '\n';
  out << "Current type:\t" << getImplementedTypes().at(settings._type_hash)
      << '\n';
  out << "========================\n";
  return out;
}

static bool parseAddr(std::string_view ip_addr, Settings& settings)
{
  std::array<uint8_t, kIpAddrOctetAmount> addr{0};

  for (auto& octet : addr) {
    const size_t delimeter_pos = ip_addr.find('.');
    std::string_view substr_octet = ip_addr.substr(0, delimeter_pos);

    auto [ptr, ec] =
        std::from_chars(substr_octet.begin(), substr_octet.end(), octet);

    if (ec != std::errc() || ptr != substr_octet.end()) {
      return false;
    }

    ip_addr.remove_prefix(delimeter_pos + 1);
  }

  settings.setAddr(addr);
  return true;
}

inline static bool parsePort(std::string_view port, Settings& settings)
{
  size_t port_number{};
  auto [ptr, ec] = std::from_chars(port.begin(), port.end(), port_number);
  if (ec != std::errc() || ptr != port.end()) {
    return false;
  }
  settings.setPort(port_number);
  return true;
}

inline static bool parseIndex(std::string_view index, Settings& settings)
{
  size_t index_number{};
  auto [ptr, ec] = std::from_chars(index.begin(), index.end(), index_number);
  if (ec != std::errc() || ptr != index.end()) {
    return false;
  }
  settings.setPort(index_number);
  return true;
}

static void changeType(Settings& settings)
{
  clearScreen();
  std::string string_input;
  const auto& implemented_types_reference = getImplementedTypes();

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

    if (hashed_input == kQuitMenuHash) {
      return;
    }

    if (std::cin.good() && implemented_types_reference.contains(hashed_input)) {
      settings.setTypeHash(hashed_input);
      settings.setTypeEnum(getEnumType().at(hashed_input));
      return;
    }

    clearCinBuffer();
    std::cerr << "Wrong input, try again: ";
  }
}

static void changeRole(Settings& settings)
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
    if (std::hash<std::string_view>{}(string_input) == kQuitMenuHash) {
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

template <typename T, EnumTypes EnumType>
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
inline std::from_chars_result convertAnyType<std::string, EnumTypes::String>(
    std::string_view string_input, any_type& emplace_element)
{
  emplace_element.emplace<static_cast<size_t>(EnumTypes::String)>(string_input);
  return {.ptr = string_input.end(), .ec = std::errc()};
}

//need to support 'true'/'false' input
static inline std::from_chars_result convertAnyTypeBool(
    std::string_view string_input, any_type& emplace_element,
    size_t hashed_input)
{
  std::from_chars_result conv_result(string_input.end());

  bool result = hashed_input == kTrueHashSymbolic;

  if (!result && hashed_input != kFalseHashSymbolic) {
    size_t input{};
    conv_result =
        std::from_chars(string_input.begin(), string_input.end(), input);
    result = input == 1;
  }

  emplace_element.emplace<static_cast<size_t>(EnumTypes::Bool)>(result);
  return conv_result;
}

inline static std::from_chars_result emplaceInVector(
    EnumTypes current_type, any_type& emplace_element,
    std::string_view string_input,
    size_t hashed_input)  //with hashed_input to support 'true'/'false' insert
{
  std::from_chars_result conv_result{};

  switch (current_type) {
    case EnumTypes::Int:
      conv_result =
          convertAnyType<int, EnumTypes::Int>(string_input, emplace_element);
      break;

    case EnumTypes::Float:
      conv_result = convertAnyType<float, EnumTypes::Float>(string_input,
                                                            emplace_element);
      break;
    case EnumTypes::Double:
      conv_result = convertAnyType<double, EnumTypes::Double>(string_input,
                                                              emplace_element);
      break;
    case EnumTypes::Char:
      conv_result =
          convertAnyType<char, EnumTypes::Char>(string_input, emplace_element);
      break;
    case EnumTypes::String:
      conv_result = convertAnyType<std::string, EnumTypes::String>(
          string_input, emplace_element);
      break;
    case EnumTypes::Bool: {
      conv_result =
          convertAnyTypeBool(string_input, emplace_element, hashed_input);
      break;
    }
    case EnumTypes::Int8:
      conv_result = convertAnyType<int8_t, EnumTypes::Int8>(string_input,
                                                            emplace_element);
      break;
    case EnumTypes::Int16:
      conv_result = convertAnyType<int16_t, EnumTypes::Int16>(string_input,
                                                              emplace_element);
      break;
    case EnumTypes::Int32:
      conv_result = convertAnyType<int32_t, EnumTypes::Int32>(string_input,
                                                              emplace_element);
      break;
    case EnumTypes::Int64:
      conv_result = convertAnyType<int64_t, EnumTypes::Int64>(string_input,
                                                              emplace_element);
      break;
    case EnumTypes::UInt8:
      conv_result = convertAnyType<uint8_t, EnumTypes::UInt8>(string_input,
                                                              emplace_element);
      break;
    case EnumTypes::UInt16:
      conv_result = convertAnyType<uint16_t, EnumTypes::UInt16>(
          string_input, emplace_element);
      break;
    case EnumTypes::UInt32:
      conv_result = convertAnyType<uint32_t, EnumTypes::UInt32>(
          string_input, emplace_element);
      break;
    case EnumTypes::UInt64:
      conv_result = convertAnyType<uint64_t, EnumTypes::UInt64>(
          string_input, emplace_element);
      break;
  }

  return conv_result;
}

template <size_t N>
static void enterVector(PolymorpicVector<N>& vector, Settings const& settings)
{
  PolymorpicVector<N> spare_vector;

  std::cout << "Enter " << N << "-dimensional vector of "
            << getImplementedTypes().at(settings.cgetTypeHash())
            << " or "
               "enter 'quit' if you've changed your "
               "mind.\nFormat is "
            << N << " values separated by whitespaces: ";

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

      if (hashed_input == kQuitMenuHash) {
        return;
      }

      EnumTypes const current_type = settings.cgetTypeEnum();
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

template <size_t N>
inline static void printVector(PolymorpicVector<N>& arr)
{
  for (auto& i : arr) {
    std::visit([](auto const& variant_val) { std::cout << variant_val << ' '; },
               i);
  }
  std::cout << '\n';
}

enum class ParseResult : uint8_t {
  NO_ERR,
  WRONG_FLAG,
  NO_ARGUMENT,
};

inline static ParseResult parseClArgs(
    std::unordered_map<size_t, std::string_view>& argument_map, char** argv,
    int argc)
{
  bool is_next_arg = false;
  bool first_arg = true;
  auto it = argument_map.begin();

  for (auto& argument : std::span(argv, argc)) {
    //need to skip program name as variable without warning about pointer
    //arithmetic, idk how else i can do this
    if (first_arg) {
      first_arg = false;
      continue;
    }

    if (is_next_arg) {
      is_next_arg = false;
      it->second = argument;
      continue;
    }

    it = argument_map.find(std::hash<std::string_view>{}(argument));

    if (it == argument_map.end()) {
      std::cout << argument << std::hash<std::string_view>{}(argument) << '\n';
      std::cerr << "No such flag\n";
      return ParseResult::WRONG_FLAG;
    }

    is_next_arg = true;
  }

  if (is_next_arg) {
    std::cerr << "Missed argument\n";
    return ParseResult::NO_ARGUMENT;
  }

  return ParseResult::NO_ERR;
}

int main(int argc, char* argv[])
{

  std::unordered_map<size_t, std::string_view> cl_args{
      {kAddrHash, "127.0.0.1"}, {kPortHash, "5555"}, {kRoleHash, "Client"},
      {kIndexHash, "0"},        {kLibHash, "Lib1"},
  };
  parseClArgs(cl_args, argv, argc);

  Settings command_line_options{};
  if (!parseAddr(cl_args.at(kAddrHash), command_line_options)) {
    std::cout << "Invalid IP address\n";
    return 0;
  }

  if (!parsePort(cl_args.at(kPortHash), command_line_options)) {
    std::cout << "Invalid port\n";
    return 0;
  }

  if (!parseIndex(cl_args.at(kIndexHash), command_line_options)) {
    std::cout << "Invalid index\n";
    return 0;
  }

  command_line_options.setLibName(cl_args.at(kLibHash));
  command_line_options.setRole(cl_args.at(kRoleHash));

  displayMenu();

  PolymorpicVector<kVectorDimensionsAmount> task_vector{};

  //if i have reference to static object, as I knowm there won't be any additional
  //calls to check if static object is initialized
  const std::unordered_map<size_t, MenuOptions>& menu_options =
      getMenuOptions();

  while (true) {
    std::string text_option;

    std::cout << "Your command: ";

    std::cin >> text_option;
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = menu_options.contains(input_hash);
    auto picked_option = is_correct_option ? menu_options.at(input_hash)
                                           : MenuOptions::WrongOption;
    switch (picked_option) {
      case MenuOptions::ChangeRole:
        changeRole(command_line_options);

        displayMenu();
        clearCinBuffer();
        break;

      case MenuOptions::ChangeType:
        changeType(command_line_options);

        displayMenu();
        clearCinBuffer();
        break;

      case MenuOptions::PrintSettings:
        std::cout << '\n' << command_line_options << '\n';
        break;

      case MenuOptions::PrintCurrentVector:
        printVector(task_vector);
        break;

      case MenuOptions::EnterVector:
        enterVector(task_vector, command_line_options);

        displayMenu();
        clearCinBuffer();
        break;

      case MenuOptions::QuitProgram:
        return 0;

      default:
        std::cout << "Wrong menu option, try again\n";
    }
  }
}

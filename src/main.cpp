#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

#include "display.hpp"

constexpr size_t kIpAddrOctetAmount{4};
constexpr size_t kIpAddrOctetSize{3};
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
  [[nodiscard]] std::string const& getLibName() const { return _lib_name; }
  [[nodiscard]] size_t getTypeHash() const { return _type_hash; }
  [[nodiscard]] EnumTypes getTypeEnum() const { return _type_enum; }
  [[nodiscard]] std::string const& getRole() const { return _role; }
  [[nodiscard]] size_t getPort() const { return _port; }
  [[nodiscard]] size_t getIndex() const { return _index; }
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
  for (auto const* it = settings._ip_addr.begin();
       it != settings._ip_addr.end(); ++it) {
    char delimeter = it + 1 == settings._ip_addr.end() ? ' ' : '.';
    out << static_cast<int16_t>(*it) << delimeter;
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

  size_t octet_number = 0;
  size_t len = 0;
  auto const* it_begin = ip_addr.begin();
  for (auto const* it = ip_addr.begin(); it != ip_addr.end(); ++it) {
    if (octet_number >= kIpAddrOctetAmount || len > kIpAddrOctetSize) {
      return false;
    }

    bool is_end = it + 1 == ip_addr.end();

    if (*it != '.' && !is_end) {
      len++;
      continue;
    }

    len += is_end ? 1 : 0;
    uint8_t octet = 0;

    auto [ptr, ec] = std::from_chars(it_begin, it_begin + len, octet);

    if (ec == std::errc::invalid_argument) {
      std::cout << " This is not a number.\n";
      return false;
    }
    if (ec == std::errc::result_out_of_range) {
      std::cout << "This number is larger than an int.\n";
      return false;
    }

    addr[octet_number++] = octet;
    it_begin = ptr + 1;
    len = 0;
  }
  settings.setAddr(addr);
  return true;
}

inline static bool parsePort(std::string_view port, Settings& settings)
{
  try {
    settings.setPort(std::stoull(port.data()));
    return true;
  }
  catch (std::exception& e) {
    return false;
  }
}

inline static bool parseIndex(std::string_view index, Settings& settings)
{
  try {
    settings.setIndex(std::stoull(index.data()));
    return true;
  }
  catch (std::exception& e) {
    return false;
  }
}

static void changeType(Settings& settings)
{
  clearScreen();
  std::string string_input;
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

    if (std::cin.good() && getImplementedTypes().contains(hashed_input)) {
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

inline static void emplaceInVector(EnumTypes current_type,
                                   any_type& emplace_element,
                                   std::string_view string_input,
                                   size_t hashed_input)
{
  switch (current_type) {
    case EnumTypes::Int:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Int)>(
          std::stoi(string_input.data()));
      break;
    case EnumTypes::Float:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Float)>(
          std::stof(string_input.data()));
      break;
    case EnumTypes::Double:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Double)>(
          std::stod(string_input.data()));
      break;
    case EnumTypes::Char:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Char)>(
          string_input.data()[0]);
      break;
    case EnumTypes::String:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::String)>(
          string_input.data());
      break;
    case EnumTypes::Bool: {
      bool value_to_set = kTrueHashSymbolic == hashed_input;
      if (hashed_input != kTrueHashSymbolic &&
          hashed_input != kFalseHashSymbolic) {
        value_to_set = std::stoi(string_input.data()) == 1;
      }
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Bool)>(
          value_to_set);
      break;
    }
    case EnumTypes::Int8:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Int8)>(
          static_cast<int8_t>(std::stoi(string_input.data())));
      break;
    case EnumTypes::Int16:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Int16)>(
          static_cast<int16_t>(std::stoi(string_input.data())));
      break;
    case EnumTypes::Int32:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Int32)>(
          static_cast<int32_t>(std::stol(string_input.data())));
      break;
    case EnumTypes::Int64:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::Int64)>(
          static_cast<int64_t>(std::stoll(string_input.data())));
      break;
    case EnumTypes::UInt8:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::UInt8)>(
          static_cast<uint8_t>(std::stoi(string_input.data())));
      break;
    case EnumTypes::UInt16:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::UInt16)>(
          static_cast<uint16_t>(std::stoull(string_input.data())));
      break;
    case EnumTypes::UInt32:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::UInt32)>(
          static_cast<uint32_t>(std::stoull(string_input.data())));
      break;
    case EnumTypes::UInt64:
      emplace_element.emplace<static_cast<size_t>(EnumTypes::UInt64)>(
          static_cast<uint64_t>(std::stoull(string_input.data())));
      break;
  }
}

template <size_t N>
static void enterVector(PolymorpicVector<N>& vector, Settings const& settings)
{
  clearScreen();
  PolymorpicVector<N> spare_vector;

  std::cout << "Enter " << N << "-dimensional vector of "
            << getImplementedTypes().at(settings.getTypeHash())
            << " or "
               "enter 'quit' if you've changed your "
               "mind\nformat is "
            << N << " values separated by whitespaces: ";

  std::string string_input;

  size_t index = 0;
  while (index < N) {
    std::cin >> string_input;
    std::ranges::transform(string_input, string_input.begin(), ::tolower);
    size_t hashed_input = std::hash<std::string_view>{}(string_input);

    if (hashed_input == kQuitMenuHash) {
      return;
    }

    if (std::cin.bad()) {
      clearCinBuffer();
      std::cerr << "Wrong input, try again: ";
      continue;
    }
    EnumTypes const current_type = settings.getTypeEnum();
    try {
      emplaceInVector(current_type, spare_vector[index], string_input,
                      hashed_input);
    }
    catch (std::exception& e) {
      clearCinBuffer();

      std::cout << "Error during type conversion, re-input: ";
      index = 0;
      continue;
    }
    index++;
  }
  vector = spare_vector;
}
template <size_t N>
static void printVector(PolymorpicVector<N>& arr)
{
  for (auto& i : arr) {
    std::visit([](auto const& variant_val) { std::cout << variant_val << ' '; },
               i);
    std::cout << '\n';
  }
}

int main(int argc, char* argv[])
{
  std::unordered_map<size_t, std::string_view> cl_args{
      {kAddrHash, "127.0.0.1"}, {kPortHash, "5555"}, {kRoleHash, "Client"},
      {kIndexHash, "0"},        {kLibHash, "Lib1"},
  };

  Settings command_line_options;
  int count = 1;
  while (count < argc) {
    auto it = cl_args.find(std::hash<std::string_view>{}(argv[count]));

    if (it == cl_args.end()) {
      std::cerr << "No such flag\n";
      return 0;
    }

    if (++count >= argc) {
      std::cerr << "Missed argument\n";
      return 0;
    }

    it->second = argv[count];
    count++;
  }

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

  while (true) {
    std::string text_option;

    std::cout << "Your command: ";

    std::cin >> text_option;
    std::ranges::transform(text_option, text_option.begin(), ::tolower);
    size_t input_hash = std::hash<std::string_view>{}(text_option);
    bool is_correct_option = getMenuOptions().contains(input_hash);
    auto picked_option = is_correct_option ? getMenuOptions().at(input_hash)
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

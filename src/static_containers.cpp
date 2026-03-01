#include "static_containers.hpp"
#include "hashed_values.hpp"
namespace static_containers {
std::unordered_map<size_t, std::string_view> const& getImplementedTypes()
{
  static std::unordered_map<size_t, std::string_view> const k_type_checker{
      {hashed::kIntHash, "int"},         {hashed::kFloatHash, "float"},
      {hashed::kDoubleHash, "double"},   {hashed::kCharHash, "char"},
      {hashed::kStringHash, "string"},   {hashed::kBoolHash, "bool"},

      {hashed::kInt8Hash, "int8_t"},     {hashed::kInt16Hash, "int16_t"},
      {hashed::kInt32Hash, "int32_t"},   {hashed::kInt64Hash, "int64_t"},

      {hashed::kUInt8Hash, "uint8_t"},   {hashed::kUInt16Hash, "uint16_t"},
      {hashed::kUInt32Hash, "uint32_t"}, {hashed::kUInt64Hash, "uint64_t"},
  };

  return k_type_checker;
}

std::unordered_map<size_t, EnumTypes> const& getEnumType()
{
  static std::unordered_map<size_t, EnumTypes> const enum_types_converter{
      {hashed::kIntHash, EnumTypes::Int},
      {hashed::kFloatHash, EnumTypes::Float},
      {hashed::kDoubleHash, EnumTypes::Double},
      {hashed::kCharHash, EnumTypes::Char},
      {hashed::kStringHash, EnumTypes::String},
      {hashed::kBoolHash, EnumTypes::Bool},

      {hashed::kInt8Hash, EnumTypes::Int8},
      {hashed::kInt16Hash, EnumTypes::Int16},
      {hashed::kInt32Hash, EnumTypes::Int32},
      {hashed::kInt64Hash, EnumTypes::Int64},

      {hashed::kUInt8Hash, EnumTypes::UInt8},
      {hashed::kUInt16Hash, EnumTypes::UInt16},
      {hashed::kUInt32Hash, EnumTypes::UInt32},
      {hashed::kUInt64Hash, EnumTypes::UInt64},
  };
  return enum_types_converter;
}
std::unordered_map<size_t, MenuOptions> const& getMenuOptions()
{
  static std::unordered_map<size_t, MenuOptions> const k_menu_options{
      {hashed::kNameMenuHash, MenuOptions::ChangeRole},
      {hashed::kTypeMenuHash, MenuOptions::ChangeType},
      {hashed::kVectorMenuHash, MenuOptions::EnterVector},
      {hashed::kPrintHash, MenuOptions::PrintCurrentVector},
      {hashed::kQuitMenuHash, MenuOptions::QuitProgram},
      {hashed::kEmptyQueue, MenuOptions::EmptyQueue},
      {hashed::kSettingsMenuHash, MenuOptions::PrintSettings}};

  return k_menu_options;
}
};  // namespace static_containers

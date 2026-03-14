#include "custom_types.hpp"
#include "logger.hpp"
namespace protei_types {
std::unordered_map<size_t, std::pair<EnumTypes, std::string_view>> const&
getHashToTypeInfo()
{
  static std::unordered_map<size_t,
                            std::pair<EnumTypes, std::string_view>> const
      enum_types_converter{
          {hashed::kInt, {EnumTypes::Int, "int"}},
          {hashed::kFloat, {EnumTypes::Float, "float"}},
          {hashed::kDouble, {EnumTypes::Double, "double"}},
          {hashed::kChar, {EnumTypes::Char, "char"}},
          {hashed::kString, {EnumTypes::String, "string"}},
          {hashed::kBool, {EnumTypes::Bool, "bool"}},

          {hashed::kInt8, {EnumTypes::Int8, "int8_t"}},
          {hashed::kInt16, {EnumTypes::Int16, "int16_t"}},
          {hashed::kInt32, {EnumTypes::Int32, "int32_t"}},
          {hashed::kInt64, {EnumTypes::Int64, "int64_t"}},

          {hashed::kUInt8, {EnumTypes::UInt8, "uint8_t"}},
          {hashed::kUInt16, {EnumTypes::UInt16, "uint16_t"}},
          {hashed::kUInt32, {EnumTypes::UInt32, "uint32_t"}},
          {hashed::kUInt64, {EnumTypes::UInt64, "uint64_t"}},
      };

  logger_presets::createdStaticContainer(
      "Hashed type - pair <EnumType, TypeName> - unordered_map");
  return enum_types_converter;
}
};  // namespace protei_types

#include "custom_types.hpp"
#include "logger.hpp"
namespace custom_types {
std::string_view getTypename(const any_type& value)
{
  return std::visit(
      custom_types::Visitor{
          [](const uint8_t) { return "uint8_t"; }, [](const uint16_t) { return "uint16_t"; },
          [](const uint32_t) { return "uint32_t"; }, [](const uint64_t) { return "uint64_t"; },
          [](const int8_t) { return "int8_t"; }, [](const int16_t) { return "int16_t"; },
          [](const int32_t) { return "int32_t"; }, [](const int64_t) { return "int64_t"; },
          [](const bool) { return "bool"; }, [](const float) { return "float"; },
          [](const double) { return "double"; }, [](const std::string&) { return "string"; },
          [](const char) { return "char"; }},
      value);
};  // namespace data_storage
size_t getHash(const any_type& value)
{
  return std::visit(custom_types::Visitor{[](const uint8_t) { return hashed::kUInt8; },
                                          [](const uint16_t) { return hashed::kUInt16; },
                                          [](const uint32_t) { return hashed::kUInt32; },
                                          [](const uint64_t) { return hashed::kUInt64; },
                                          [](const int8_t) { return hashed::kInt8; },
                                          [](const int16_t) { return hashed::kInt16; },
                                          [](const int32_t) { return hashed::kInt32; },
                                          [](const int64_t) { return hashed::kInt64; },
                                          [](const bool) { return hashed::kBool; },
                                          [](const float) { return hashed::kFloat; },
                                          [](const double) { return hashed::kDouble; },
                                          [](const std::string&) { return hashed::kString; }},
                    value);
};
std::unordered_map<size_t, any_type>& getDefaultValues()
{
  static std::unordered_map<size_t, any_type> type_dispatch{
      {hashed::kBool, false},
      {hashed::kChar, char{}},
      {hashed::kDouble, 0.},
      {hashed::kFloat, 0.F},
      {hashed::kInt, 0},
      {hashed::kInt16, static_cast<int16_t>(0)},
      {hashed::kInt32, static_cast<int32_t>(0)},
      {hashed::kInt64, static_cast<int64_t>(0)},
      {hashed::kInt8, static_cast<int8_t>(0)},
      {hashed::kUInt16, static_cast<uint16_t>(0)},
      {hashed::kUInt32, static_cast<uint32_t>(0)},
      {hashed::kUInt64, static_cast<uint64_t>(0)},
      {hashed::kUInt8, static_cast<uint8_t>(0)},
      {hashed::kString, ""},
  };

  logging::SingleThreadPresets::createdStaticContainer("Hash - default_value - unordered_map");
  return type_dispatch;
}
}  // namespace custom_types

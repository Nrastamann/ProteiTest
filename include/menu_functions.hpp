#pragma once
#include <iostream>
#include <unordered_map>
#include "data_pool.hpp"

#include "custom_types.hpp"
#include "settings.hpp"

struct NonConstTag {};

namespace hashed {
inline size_t const kQuit = std::hash<std::string_view>{}("quit");
}  // namespace hashed

namespace protei_types {
const std::unordered_map<protei_types::EnumTypes, protei_types::any_type>&
getDefaultValues();
}  // namespace protei_types

namespace hashed {
inline size_t const kTrueSymbolic = std::hash<std::string_view>{}("true");
inline size_t const kFalseSymbolic = std::hash<std::string_view>{}("false");
}  // namespace hashed

namespace menu_functions_protei {

template <typename T>
concept isPartOf = std::is_assignable_v<protei_types::any_type, T>;

void changeType(AppSettings& settings);
void changeName(AppSettings& settings);

void enterVector(DataPool& vector, AppSettings const& settings);
void emptyQueue(DataPool& data_pool, NonConstTag);

void printVector(DataPool& arr, NonConstTag);

inline void quit(AppSettings& settings)
{
  settings.setShouldClose();
}

inline void printCurrentAppSettings(const AppSettings& settings)
{
  ui_protei::printAppSettings(settings);
}

inline void wrongOption()
{
  std::cout << "Wrong menu option, try again\n";
}
}  // namespace menu_functions_protei

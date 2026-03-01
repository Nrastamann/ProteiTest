#pragma once
#include <iostream>
#include <unordered_map>
#include "data_pool.hpp"
#include "settings.hpp"
#include "static_containers.hpp"
#include "utility.hpp"

struct NonConstTag {};

namespace static_containers {
const std::unordered_map<static_containers::EnumTypes, any_type>&
getDefaultValues();
}  // namespace static_containers

namespace hashed {
inline size_t const kTrueSymbolic = std::hash<std::string_view>{}("true");
inline size_t const kFalseSymbolic = std::hash<std::string_view>{}("false");
}  // namespace hashed

namespace menu_functions_protei {

template <typename T>
concept isPartOf = std::is_assignable_v<any_type, T>;

void changeType(AppSettings& settings);
void changeName(AppSettings& settings);

void enterVector(DataPool& vector, AppSettings const& settings);
void emptyQueue(DataPool& data_pool, NonConstTag);

void printVector(DataPool& arr, NonConstTag);

inline void quit(AppSettings& settings)
{
  settings.setShouldClose();
}

inline void printCurrentAppSettings(AppSettings& settings)
{
  ui_protei::printAppSettings(settings);
}

inline void wrongOption()
{
  std::cout << "Wrong menu option, try again\n";
}
}  // namespace menu_functions_protei

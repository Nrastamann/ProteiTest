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

namespace custom_types {
const std::unordered_map<EnumTypes, any_type>& getDefaultValues();
}  // namespace custom_types

namespace hashed {
inline size_t const kTrueSymbolic = std::hash<std::string_view>{}("true");
inline size_t const kFalseSymbolic = std::hash<std::string_view>{}("false");
}  // namespace hashed

namespace menu_functions {

template <typename T>
concept isPartOf = std::is_assignable_v<custom_types::any_type, T>;

void changeType(AppSettings& settings);
void changeName(AppSettings& settings);

void enterVector(data_storage::DataPool& vector, AppSettings const& settings);
void emptyQueue(data_storage::DataPool& data_pool, NonConstTag);

void printVector(data_storage::DataPool& arr, NonConstTag);
void sendToServer(data_storage::DataPool& datapool,
                  const AppSettings& settings);

std::from_chars_result emplaceInVector(custom_types::any_type& emplace_element,
                                       std::string_view string_input,
                                       size_t hashed_input);
inline void emptyFunction() {}

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
}  // namespace menu_functions

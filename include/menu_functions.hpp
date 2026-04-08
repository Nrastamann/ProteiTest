#pragma once
#include <iostream>
#include "data_pool.hpp"

#include "settings.hpp"

struct NonConstTag {};
struct BothNonConstTag {};

namespace hashed {
inline size_t const kQuit = std::hash<std::string_view>{}("quit");
}  // namespace hashed

namespace hashed {
inline size_t const kTrueSymbolic = std::hash<std::string_view>{}("true");
inline size_t const kFalseSymbolic = std::hash<std::string_view>{}("false");
}  // namespace hashed

namespace menu_functions {
void moveX(AppSettings& settings);
void sms(data_storage::DataPool& sms, AppSettings& settings, BothNonConstTag);
void activate(data_storage::DataPool& data_pool, AppSettings& settings, BothNonConstTag);
void status(data_storage::DataPool& sms, AppSettings& settings, BothNonConstTag);

//std::from_chars_result emplaceInVector(utility::any_type& emplace_element,
//                                       std::string_view string_input, size_t hashed_input);
inline void emptyFunction() {}

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
}  // namespace menu_functions

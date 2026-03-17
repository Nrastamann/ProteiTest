#include "menu.hpp"
#include "custom_types.hpp"
#include "logger.hpp"

namespace custom_types {

std::unordered_map<size_t, MenuOptions> const& getMenuOptions()
{

  static std::unordered_map<size_t, MenuOptions> const k_menu_options{
      {hashed::kNameMenu, MenuOptions::ChangeRole},
      {hashed::kTypeMenu, MenuOptions::ChangeType},
      {hashed::kVectorMenu, MenuOptions::EnterVector},
      {hashed::kPrint, MenuOptions::PrintCurrentVector},
      {hashed::kQuit, MenuOptions::QuitProgram},
      {hashed::kExit, MenuOptions::QuitProgram},
      {hashed::kEmptyQueue, MenuOptions::EmptyQueue},
      {hashed::kSettingsMenu, MenuOptions::PrintSettings},
      {hashed::kSend, MenuOptions::SendToServer},
      {hashed::kClear, MenuOptions::EmptyFunction}};

  logging::logger_presets::createdStaticContainer(
      "Hash - MenuOptions unordered_map");
  return k_menu_options;
}
}  // namespace custom_types

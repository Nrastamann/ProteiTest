#include "menu.hpp"
#include "static_containers.hpp"

namespace hashed {
inline size_t const kNameMenu = std::hash<std::string_view>{}("name");
inline size_t const kTypeMenu = std::hash<std::string_view>{}("type");
inline size_t const kVectorMenu = std::hash<std::string_view>{}("vector");
inline size_t const kPrint = std::hash<std::string_view>{}("print");
inline size_t const kEmptyQueue = std::hash<std::string_view>{}("empty");
inline size_t const kSettingsMenu = std::hash<std::string_view>{}("settings");
}  // namespace hashed

namespace static_containers {

std::unordered_map<size_t, MenuOptions> const& getMenuOptions()
{
  static std::unordered_map<size_t, MenuOptions> const k_menu_options{
      {hashed::kNameMenu, MenuOptions::ChangeRole},
      {hashed::kTypeMenu, MenuOptions::ChangeType},
      {hashed::kVectorMenu, MenuOptions::EnterVector},
      {hashed::kPrint, MenuOptions::PrintCurrentVector},
      {hashed::kQuit, MenuOptions::QuitProgram},
      {hashed::kEmptyQueue, MenuOptions::EmptyQueue},
      {hashed::kSettingsMenu, MenuOptions::PrintSettings}};

  return k_menu_options;
}
}  // namespace static_containers

void Menu::callFunctionVariant(const protei_function& function,
                               FunctionArgs& arguments) const
{
  std::visit(
      Visitor{
          [&settings = arguments._cl_args](
              const std::function<void(AppSettings&)>& fn) { fn(settings); },
          [&vec = arguments._dataPool, &settings = arguments._cl_args](
              const std::function<void(DataPool&, const AppSettings&)>& fn) {
            fn(vec, settings);
          },
          [&vec = arguments._dataPool](
              const std::function<void(DataPool&, NonConstTag)>& fn) {
            fn(vec, {});
          },
          [&vec = arguments._dataPool](
              const std::function<void(const DataPool&)>& fn) { fn(vec); },
          [](const std::function<void()>& fn) { fn(); }},
      function);
}

Menu::cref_function_container Menu::getContainer()
{
  using static_containers::MenuOptions;
  static function_container functions{
      {MenuOptions::ChangeType, MenuItem{menu_functions_protei::changeType,
                                         pre_hooks_protei::defaultClear,
                                         post_hooks_protei::defaultClear}},
      {MenuOptions::EmptyQueue,
       MenuItem{menu_functions_protei::emptyQueue, defaultEmpty,
                post_hooks_protei::clearBuffer}},

      {MenuOptions::ChangeRole, MenuItem{menu_functions_protei::changeName,
                                         pre_hooks_protei::defaultClear,
                                         post_hooks_protei::defaultClear}},
      {MenuOptions::EnterVector, MenuItem{menu_functions_protei::enterVector,
                                          pre_hooks_protei::defaultClear,
                                          post_hooks_protei::defaultClear}},
      {MenuOptions::PrintCurrentVector,
       MenuItem{menu_functions_protei::printVector, defaultEmpty,
                post_hooks_protei::clearBuffer}},
      {MenuOptions::PrintSettings,
       MenuItem{menu_functions_protei::printCurrentAppSettings, defaultEmpty,
                post_hooks_protei::clearBuffer}},
      {MenuOptions::WrongOption,
       MenuItem{menu_functions_protei::wrongOption, defaultEmpty,
                post_hooks_protei::clearBuffer}},
      {MenuOptions::QuitProgram,
       MenuItem{menu_functions_protei::quit, defaultEmpty,
                post_hooks_protei::clearBuffer}},
  };
  return functions;
}

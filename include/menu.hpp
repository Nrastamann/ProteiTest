#pragma once

#include <functional>
#include "data_pool.hpp"
#include "hooks.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"
#include "settings.hpp"

namespace hashed {
inline size_t const kNameMenu = std::hash<std::string_view>{}("name");
inline size_t const kTypeMenu = std::hash<std::string_view>{}("type");
inline size_t const kVectorMenu = std::hash<std::string_view>{}("vector");
inline size_t const kPrint = std::hash<std::string_view>{}("print");
inline size_t const kEmptyQueue = std::hash<std::string_view>{}("empty");
inline size_t const kSettingsMenu = std::hash<std::string_view>{}("settings");
inline size_t const kExit = std::hash<std::string_view>{}("exit");
inline size_t const kSend = std::hash<std::string_view>{}("send");
}  // namespace hashed

using polymorphic_function = std::variant<
    std::function<void(AppSettings&)>,
    std::function<void(data_storage::DataPool&, const AppSettings&)>,
    std::function<void(data_storage::DataPool&, NonConstTag)>,
    std::function<void(const data_storage::DataPool&)>, std::function<void()>>;

struct FunctionArgs {
  ~FunctionArgs() = default;
  FunctionArgs(const FunctionArgs&) = delete;
  FunctionArgs(FunctionArgs&&) = delete;
  FunctionArgs& operator=(const FunctionArgs&) = delete;
  FunctionArgs& operator=(FunctionArgs&&) = delete;
  FunctionArgs(AppSettings& cl_args, data_storage::DataPool& data_pool)
      : _cl_args(cl_args), _dataPool(data_pool)
  {
  }
  AppSettings& _cl_args;
  data_storage::DataPool& _dataPool;
};

/**
 * struct MenuItem - MenuItem - Abstraction of menu option call, it includes
 * pre-hook call, function and post_hook function call
 * Инвариант: 
 * Вызываемая функция не должна иметь возвращаемое значение
 * Тип вызываемых функций, должен быть включен в объекты protei_hook и protei_function
 */
struct MenuItem {
  using menu_hook = menu_hooks::menu_hook;
  explicit MenuItem(polymorphic_function fn,
                    menu_hook pre_hook = menu_hooks::defaultEmpty,
                    menu_hook post_hook = menu_hooks::defaultEmpty)
      : _pre_hook(std::move(pre_hook)),
        _fn(std::move(fn)),
        _post_hook(std::move(post_hook))
  {
  }

  ~MenuItem() = default;
  MenuItem(const MenuItem&) = default;
  MenuItem(MenuItem&&) = default;
  MenuItem& operator=(const MenuItem&) = default;
  MenuItem& operator=(MenuItem&&) = default;

  menu_hook _pre_hook;
  polymorphic_function _fn;
  menu_hook _post_hook;
};

class Menu {
  using function_container =
      std::unordered_map<custom_types::MenuOptions, MenuItem>;

  using ref_function_container = function_container&;
  using cref_function_container = const function_container&;

 public:
  ~Menu() = default;
  Menu() = default;
  Menu(const Menu&) = delete;
  Menu(Menu&&) = delete;
  Menu& operator=(const Menu&) = delete;
  Menu& operator=(Menu&&) = delete;

  template <typename U, typename V>
  void callFunction(custom_types::MenuOptions option,
                    [[maybe_unused]] U&& pre_hook_arg,
                    [[maybe_unused]] V&& post_hook_arg,
                    FunctionArgs& arguments) const
  {
    logging::logger_presets::functionCall();
    const MenuItem& menu_item = _items.at(option);

    menu_hooks::callHook(menu_item._pre_hook);
    callFunctionVariant(menu_item._fn, arguments);
    menu_hooks::callHook(menu_item._post_hook);
  }

  template <typename U, typename V>
  void menuTask([[maybe_unused]] U&& pre_hook_arg,
                [[maybe_unused]] V&& post_hook_arg,
                FunctionArgs& arguments) const
  {
    logging::logger_presets::functionCall();

    std::string text_option;

    logging::Logger::writeToLogNCl<config::LogVerbosity::Debug>(
        "Your command: ");

    std::cin >> text_option;
    logging::Logger::writeToLog<config::LogVerbosity::Debug>(text_option);
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = _menu_options.contains(input_hash);

    custom_types::MenuOptions picked_option =
        is_correct_option ? _menu_options.at(input_hash)
                          : custom_types::MenuOptions::WrongOption;

    callFunction(picked_option, 0, 0, arguments);
  }

 private:
  static void callFunctionVariant(const polymorphic_function& function,
                                  const FunctionArgs& arguments)
  {
    logging::logger_presets::functionCall();

    std::visit(
        custom_types::Visitor{
            [&settings = arguments._cl_args](
                const std::function<void(AppSettings&)>& fn) { fn(settings); },
            [&vec = arguments._dataPool, &settings = arguments._cl_args](
                const std::function<void(data_storage::DataPool&,
                                         const AppSettings&)>& fn) {
              fn(vec, settings);
            },
            [&vec = arguments._dataPool](
                const std::function<void(data_storage::DataPool&, NonConstTag)>&
                    fn) { fn(vec, {}); },
            [&vec = arguments._dataPool](
                const std::function<void(const data_storage::DataPool&)>& fn) {
              fn(vec);
            },
            [](const std::function<void()>& fn) { fn(); }},
        function);
  }

  static cref_function_container getContainer()
  {
    using custom_types::MenuOptions;
    static function_container functions{
        {MenuOptions::ChangeType,
         MenuItem{menu_functions::changeType,
                  menu_hooks::pre_hooks_protei::defaultClear,
                  menu_hooks::post_hooks_protei::defaultClear}},
        {MenuOptions::EmptyQueue,
         MenuItem{menu_functions::emptyQueue, menu_hooks::defaultEmpty,
                  menu_hooks::post_hooks_protei::clearBuffer}},

        {MenuOptions::ChangeRole,
         MenuItem{menu_functions::changeName,
                  menu_hooks::pre_hooks_protei::defaultClear,
                  menu_hooks::post_hooks_protei::defaultClear}},
        {MenuOptions::EnterVector,
         MenuItem{menu_functions::enterVector,
                  menu_hooks::pre_hooks_protei::defaultClear,
                  menu_hooks::post_hooks_protei::defaultClear}},
        {MenuOptions::PrintCurrentVector,
         MenuItem{menu_functions::printVector, menu_hooks::defaultEmpty,
                  menu_hooks::post_hooks_protei::clearBuffer}},
        {MenuOptions::PrintSettings,
         MenuItem{menu_functions::printCurrentAppSettings,
                  menu_hooks::defaultEmpty,
                  menu_hooks::post_hooks_protei::clearBuffer}},
        {MenuOptions::WrongOption,
         MenuItem{menu_functions::wrongOption, menu_hooks::defaultEmpty,
                  menu_hooks::post_hooks_protei::clearBuffer}},
        {MenuOptions::QuitProgram,
         MenuItem{menu_functions::quit, menu_hooks::defaultEmpty,
                  menu_hooks::post_hooks_protei::clearBuffer}},
        {MenuOptions::SendToServer,
         MenuItem{menu_functions::sendToServer, menu_hooks::defaultEmpty,
                  menu_hooks::post_hooks_protei::clearBuffer}},

    };
    logging::logger_presets::createdStaticContainer(
        "MenuOptions - MenuItem unordered_map");

    return functions;
  }

  cref_function_container _items = getContainer();
  const std::unordered_map<size_t, custom_types::MenuOptions>& _menu_options =
      custom_types::getMenuOptions();
};

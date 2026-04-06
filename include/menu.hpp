#pragma once
#include "data_pool.hpp"
#include "hooks.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"
#include "settings.hpp"

namespace hashed {
inline size_t const kExit = std::hash<std::string_view>{}("exit");
inline size_t const kActivate = std::hash<std::string_view>{}("activate");
inline size_t const kMove = std::hash<std::string_view>{}("move");
inline size_t const kSMS = std::hash<std::string_view>{}("sms");
inline size_t const kStatus = std::hash<std::string_view>{}("status");
inline size_t const kWrongInput = std::hash<std::string_view>{}("wrong");
inline size_t const kClear = std::hash<std::string_view>{}("clear");
}  // namespace hashed

using polymorphic_function =
    std::variant<std::function<void(AppSettings&)>,
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
  explicit MenuItem(polymorphic_function fn, menu_hook pre_hook = menu_hooks::defaultEmpty,
                    menu_hook post_hook = menu_hooks::defaultEmpty)
      : _pre_hook(std::move(pre_hook)), _fn(std::move(fn)), _post_hook(std::move(post_hook))
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
  using function_container = std::unordered_map<size_t, MenuItem>;

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
  void callFunction(size_t hash, [[maybe_unused]] U&& pre_hook_arg,
                    [[maybe_unused]] V&& post_hook_arg, FunctionArgs& arguments) const
  {
    logging::SingleThreadPresets::functionCall();
    auto it = _items.find(hash);

    const MenuItem& menu_item =
        it == _items.end() ? _items.at(hashed::kWrongInput) : it->second;

    menu_hooks::callHook(menu_item._pre_hook);
    callFunctionVariant(menu_item._fn, arguments);
    menu_hooks::callHook(menu_item._post_hook);
  }

  template <typename U, typename V>
  void menuTask([[maybe_unused]] U&& pre_hook_arg, [[maybe_unused]] V&& post_hook_arg,
                FunctionArgs& arguments) const
  {
    logging::SingleThreadPresets::functionCall();

    std::string text_option;

    std::cout << "Your command: \n";
    logging::SingleThreadLogger::writeToLog<config::LogVerbosity::Info>("Your command: ");

    std::cin >> text_option;
    logging::SingleThreadLogger::writeToLog<config::LogVerbosity::Debug>(text_option);
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    callFunction(input_hash, 0, 0, arguments);
  }

 private:
  static void callFunctionVariant(const polymorphic_function& function,
                                  const FunctionArgs& arguments)
  {
    logging::SingleThreadPresets::functionCall();

    std::visit(
        custom_types::Visitor{
            [&settings = arguments._cl_args](const std::function<void(AppSettings&)>& fn) {
              fn(settings);
            },
            [&vec = arguments._dataPool, &settings = arguments._cl_args](
                const std::function<void(data_storage::DataPool&, const AppSettings&)>& fn) {
              fn(vec, settings);
            },
            [&vec = arguments._dataPool](
                const std::function<void(data_storage::DataPool&, NonConstTag)>& fn) {
              fn(vec, {});
            },
            [&vec = arguments._dataPool](
                const std::function<void(const data_storage::DataPool&)>& fn) { fn(vec); },
            [](const std::function<void()>& fn) { fn(); }},
        function);
  }

  static cref_function_container getContainer()
  {
    static function_container functions{

        {hashed::kWrongInput, MenuItem{menu_functions::wrongOption, menu_hooks::defaultEmpty,
                                       menu_hooks::post_hooks_protei::clearBuffer}},
        {hashed::kQuit, MenuItem{menu_functions::quit, menu_hooks::defaultEmpty,
                                 menu_hooks::post_hooks_protei::clearBuffer}},
        {hashed::kExit, MenuItem{menu_functions::quit, menu_hooks::defaultEmpty,
                                 menu_hooks::post_hooks_protei::clearBuffer}},
        {hashed::kClear, MenuItem{menu_functions::emptyFunction, menu_hooks::defaultEmpty,
                                  menu_hooks::post_hooks_protei::defaultClear}},
    };

    logging::SingleThreadPresets::createdStaticContainer(
        "MenuOptions - MenuItem unordered_map");

    return functions;
  }

  cref_function_container _items = getContainer();
};

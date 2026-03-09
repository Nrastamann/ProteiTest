#pragma once

#include <functional>
#include "data_pool.hpp"
#include "hooks.hpp"
#include "logger.hpp"
#include "menu_functions.hpp"
#include "settings.hpp"

using protei_function =
    std::variant<std::function<void(AppSettings&)>,
                 std::function<void(DataPool&, const AppSettings&)>,
                 std::function<void(DataPool&, NonConstTag)>,
                 std::function<void(const DataPool&)>, std::function<void()>>;

struct FunctionArgs {
  ~FunctionArgs() = default;
  FunctionArgs(const FunctionArgs&) = delete;
  FunctionArgs(FunctionArgs&&) = delete;
  FunctionArgs& operator=(const FunctionArgs&) = delete;
  FunctionArgs& operator=(FunctionArgs&&) = delete;
  FunctionArgs(AppSettings& cl_args, DataPool& data_pool)
      : _cl_args(cl_args), _dataPool(data_pool)
  {
  }
  AppSettings& _cl_args;
  DataPool& _dataPool;
};

/**
 * struct MenuItem - MenuItem - Abstraction of menu option call, it includes
 * pre-hook call, function and post_hook function call
 * Инвариант: 
 * Вызываемая функция не должна иметь возвращаемое значение
 * Тип вызываемых функций, должен быть включен в объекты protei_hook и protei_function
 */
struct MenuItem {
  explicit MenuItem(protei_function fn, protei_hook pre_hook = defaultEmpty,
                    protei_hook post_hook = defaultEmpty)
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

  protei_hook _pre_hook;
  protei_function _fn;
  protei_hook _post_hook;
};

class Menu {
  using function_container =
      std::unordered_map<static_containers::MenuOptions, MenuItem>;

  using ref_function_container = function_container&;
  using cref_function_container = const function_container&;
  cref_function_container getContainer();
  void callFunctionVariant(const protei_function& function,
                           FunctionArgs& arguments) const;

 public:
  ~Menu() = default;
  Menu() = default;
  Menu(const Menu&) = delete;
  Menu(Menu&&) = delete;
  Menu& operator=(const Menu&) = delete;
  Menu& operator=(Menu&&) = delete;

  template <typename U, typename V>
  void callFunction(static_containers::MenuOptions option,
                    [[maybe_unused]] U&& pre_hook_arg,
                    [[maybe_unused]] V&& post_hook_arg,
                    FunctionArgs& arguments) const
  {
    logger_presets::functionCall();
    const MenuItem& menu_item = _items.at(option);

    callHook(menu_item._pre_hook);
    callFunctionVariant(menu_item._fn, arguments);
    callHook(menu_item._post_hook);
  }

  template <typename U, typename V>
  void menuTask([[maybe_unused]] U&& pre_hook_arg,
                [[maybe_unused]] V&& post_hook_arg,
                FunctionArgs& arguments) const
  {
    logger_presets::functionCall();

    std::string text_option;

    Logger::writeToLogNCl<config::LogVerbosity::Debug>("Your command: ");

    std::cin >> text_option;
    Logger::writeToLog<config::LogVerbosity::Debug>(text_option);
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = _menu_options.contains(input_hash);

    static_containers::MenuOptions picked_option =
        is_correct_option ? _menu_options.at(input_hash)
                          : static_containers::MenuOptions::WrongOption;

    callFunction(picked_option, 0, 0, arguments);
  }

 private:
  cref_function_container _items = getContainer();
  const std::unordered_map<size_t, static_containers::MenuOptions>&
      _menu_options = static_containers::getMenuOptions();
};

#pragma once

#include <functional>
#include <unordered_map>
#include "hooks.hpp"
#include "menu_functions.hpp"
#include "settings.hpp"
#include "static_containers.hpp"
#include "utility.hpp"

using protei_function = std::variant<
    std::function<void(Settings&)>,
    std::function<void(PolymorphicVector<kVectorDimensionsAmount>&,
                       const Settings&)>,
    std::function<void(const PolymorphicVector<kVectorDimensionsAmount>&)>,
    std::function<void()>>;

struct FunctionArgs {
  ~FunctionArgs() = default;
  FunctionArgs(const FunctionArgs&) = delete;
  FunctionArgs(FunctionArgs&&) = delete;
  FunctionArgs& operator=(const FunctionArgs&) = delete;
  FunctionArgs& operator=(FunctionArgs&&) = delete;
  FunctionArgs(Settings& cl_args,
               PolymorphicVector<kVectorDimensionsAmount>& vector)
      : _cl_args(cl_args), _vector(vector)
  {
  }

  Settings& _cl_args;
  PolymorphicVector<kVectorDimensionsAmount>& _vector;
};

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
  static cref_function_container getContainer()
  {
    using static_containers::MenuOptions;
    static function_container functions{
        {MenuOptions::ChangeType, MenuItem{menu_functions_protei::changeType,
                                           pre_hooks_protei::defaultClear,
                                           post_hooks_protei::defaultClear}},
        {MenuOptions::ChangeRole, MenuItem{menu_functions_protei::changeRole,
                                           pre_hooks_protei::defaultClear,
                                           post_hooks_protei::defaultClear}},
        {MenuOptions::EnterVector, MenuItem{menu_functions_protei::enterVector,
                                            pre_hooks_protei::defaultClear,
                                            post_hooks_protei::defaultClear}},
        {MenuOptions::PrintCurrentVector,
         MenuItem{menu_functions_protei::printVector, defaultEmpty,
                  post_hooks_protei::clearBuffer}},
        {MenuOptions::PrintSettings,
         MenuItem{menu_functions_protei::printCurrentSettings, defaultEmpty,
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
    const MenuItem& menu_item = _items.at(option);

    std::visit(Visitor{[](const std::function<void()>& fn) { fn(); }},
               menu_item._pre_hook);

    std::visit(
        Visitor{
            [&settings = arguments._cl_args](
                const std::function<void(Settings&)>& fn) { fn(settings); },
            [](const std::function<void()>& fn) { fn(); },
            [&vec = arguments._vector](
                const std::function<void(
                    const PolymorphicVector<kVectorDimensionsAmount>&)>& fn) {
              fn(vec);
            },
            [&vec = arguments._vector, &settings = arguments._cl_args](
                const std::function<void(
                    PolymorphicVector<kVectorDimensionsAmount>&,
                    const Settings&)>& fn) { fn(vec, settings); },
        },
        menu_item._fn);

    std::visit(Visitor{[](const std::function<void()>& fn) { fn(); }},
               menu_item._post_hook);
  }

 private:
  cref_function_container _items = getContainer();
};

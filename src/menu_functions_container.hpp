#pragma once

#include <functional>
#include <unordered_map>
#include "menu_functions.hpp"
#include "static_containers.hpp"

using protei_function = std::variant<
    std::function<void(Settings&)>,
    std::function<void(PolymorphicVector<kVectorDimensionsAmount>&,
                       const Settings&)>,
    std::function<void(const PolymorphicVector<kVectorDimensionsAmount>&)>,
    std::function<void()>>;

class MenuContainer {
 public:
  using function_container =
      std::unordered_map<static_containers::MenuOptions, protei_function>;

  using ref_function_container = function_container&;
  using cref_function_container = const function_container&;

  ~MenuContainer() = default;
  MenuContainer() = default;
  MenuContainer(const MenuContainer&) = delete;
  MenuContainer(MenuContainer&&) = delete;
  MenuContainer& operator=(const MenuContainer&) = delete;
  MenuContainer& operator=(MenuContainer&&) = delete;

  const protei_function& getFunction(static_containers::MenuOptions option)
  {
    return _ref.at(option);
  }

 private:
  cref_function_container _ref = getContainer();
  static cref_function_container getContainer()
  {

    using static_containers::MenuOptions;
    static function_container functions{
        {MenuOptions::ChangeType, changeType},
        {MenuOptions::ChangeRole, changeRole},
        {MenuOptions::EnterVector, enterVector},
        {MenuOptions::PrintCurrentVector, printVector},
        {MenuOptions::PrintSettings, printCurrentSettings},
        {MenuOptions::WrongOption, wrongOption},
        {MenuOptions::QuitProgram, quit},
    };

    return functions;
  }
};

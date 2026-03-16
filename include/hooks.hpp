#pragma once
#include <functional>
#include <variant>
#include "custom_types.hpp"
#include "display.hpp"
#include "logger.hpp"

namespace menu_hooks {
using menu_hook = std::variant<std::function<void()>>;
inline void callHook(const menu_hook& hook)
{
  logging::logger_presets::functionCall();
  std::visit(
      custom_types::Visitor{[](const std::function<void()>& fn) { fn(); }},
      hook);
}

inline void defaultEmpty() {}

namespace pre_hooks_protei {
inline void defaultClear()
{
  logging::logger_presets::functionCall();
  display::clearCinBuffer();
  display::clearScreen();
}
}  // namespace pre_hooks_protei

namespace post_hooks_protei {
inline void defaultClear()
{
  logging::logger_presets::functionCall();
  display::clearCinBuffer();
  display::clearScreen();
  display::displayMenu();
}

inline void clearBuffer()
{
  logging::logger_presets::functionCall();
  display::clearCinBuffer();
}
}  // namespace post_hooks_protei
};  // namespace menu_hooks

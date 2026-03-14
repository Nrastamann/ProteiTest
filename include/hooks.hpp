#pragma once
#include <functional>
#include <variant>
#include "custom_types.hpp"
#include "display.hpp"
#include "logger.hpp"

using protei_hook = std::variant<std::function<void()>>;
inline void callHook(const protei_hook& hook)
{
  logger_presets::functionCall();
  std::visit(
      protei_types::Visitor{[](const std::function<void()>& fn) { fn(); }},
      hook);
}

inline void defaultEmpty() {}

namespace pre_hooks_protei {
inline void defaultClear()
{
  logger_presets::functionCall();
  ui_protei::clearCinBuffer();
  ui_protei::clearScreen();
}
}  // namespace pre_hooks_protei

namespace post_hooks_protei {
inline void defaultClear()
{
  logger_presets::functionCall();
  ui_protei::clearCinBuffer();
  ui_protei::clearScreen();
  ui_protei::displayMenu();
}

inline void clearBuffer()
{
  logger_presets::functionCall();
  ui_protei::clearCinBuffer();
}
}  // namespace post_hooks_protei

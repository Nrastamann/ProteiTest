#pragma once
#include <functional>
#include <variant>
#include "display.hpp"
#include "logger.hpp"
#include "utility.hpp"

using protei_hook = std::variant<std::function<void()>>;
inline void callHook(const protei_hook& hook)
{
  logger_presets::functionCall();
  std::visit(Visitor{[](const std::function<void()>& fn) { fn(); }}, hook);
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

#pragma once
#include <functional>
#include <variant>
#include "display.hpp"

using protei_hook = std::variant<std::function<void()>>;

inline void defaultEmpty() {}

namespace pre_hooks_protei {
inline void defaultClear()
{
  ui_protei::clearCinBuffer();
  ui_protei::clearScreen();
}
}  // namespace pre_hooks_protei

namespace post_hooks_protei {
inline void defaultClear()
{
  ui_protei::clearCinBuffer();
  ui_protei::clearScreen();
  ui_protei::displayMenu();
}

inline void clearBuffer()
{
  ui_protei::clearCinBuffer();
}
}  // namespace post_hooks_protei

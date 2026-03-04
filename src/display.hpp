#pragma once
#include <iostream>
#include <limits>
#include "io_manager.hpp"

namespace ui_protei {
void displayMenu();
inline void clearScreen()
{
  protei_io::io().cout() << "\033[2J\033[1;1H";
}
inline static void clearCinBuffer()
{
  auto& in = protei_io::io().cin();
  in.clear();
  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
}  // namespace ui_protei

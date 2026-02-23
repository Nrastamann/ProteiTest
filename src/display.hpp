#pragma once
#include <iostream>
#include <limits>

void displayMenu();
inline void clearScreen()
{
  std::cout << "\033[2J\033[1;1H";
}
inline static void clearCinBuffer()
{
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

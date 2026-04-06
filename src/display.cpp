#include "display.hpp"

namespace display {
void displayMenu()
{
  clearScreen();
  std::cout << "\tProtei Practice Programm\t\n";

  std::string_view hint_text("Pick option, and after that you'll see the description\n");
  std::string delimeter(hint_text.length() - 1, '=');

  std::cout << hint_text << delimeter << '\n';

  std::cout << "Status:\t\tPrint current status\n";
  std::cout << "Activate:\t\tActivate your device for transmitting data\n";
  std::cout << "SMS:\t\tSend SMS to other UE\n";
  std::cout << "Clear:\t\tClear screen\n";
  std::cout << "Quit/Exit:\t\tQuit the program\n";
  std::cout << delimeter << '\n';
}
}  // namespace display

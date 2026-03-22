#include "display.hpp"

namespace display {
void displayMenu()
{
  clearScreen();
  std::cout << "\tProtei Practice Programm\t\n";

  std::string_view hint_text("Pick option, and after that you'll see the description\n");
  std::string delimeter(hint_text.length() - 1, '=');

  std::cout << hint_text << delimeter << '\n';

  std::cout << "Name\t\tenter your name\n";
  std::cout << "Send\t\tsend front vector to server and push recieved vector to"
               "the back (supports multiple server connection)\n";
  std::cout << "Type\t\tenter type\n";
  std::cout << "Clear\t\tclear screen\n";
  std::cout << "Settings:\tprint settings\n";
  std::cout << "Empty:\t\tempty datapool and print it\n";
  std::cout << "Vector:\t\tenter 4-dimensional vector\n";
  std::cout << "Print:\t\tprint 4-dimensional vector\n";
  std::cout << "Quit:\t\tquit the program\n";
  std::cout << delimeter << '\n';
}
}  // namespace display

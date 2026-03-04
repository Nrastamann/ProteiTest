#include "display.hpp"
#include "io_manager.hpp"

namespace ui_protei {
void displayMenu()
{
  clearScreen();
  auto& out = protei_io::io().cout();
  out << "\tProtei Practice Programm\t\n";

  std::string_view hint_text(
      "Pick option, and after that you'll see the description\n");
  std::string delimeter(hint_text.length() - 1, '=');

  out << hint_text << delimeter << '\n';

  out << "Name\t\tenter your name\n";
  out << "Type\t\tenter type\n";
  out << "Settings:\tprint settings\n";
  out << "Empty:\t\tempty datapool and print it\n";
  out << "Vector:\t\tenter 4-dimensional vector\n";
  out << "Print:\t\tprint 4-dimensional vector\n";
  out << "Quit:\t\tquit the program\n";
  out << delimeter << '\n';
}
}  // namespace ui_protei

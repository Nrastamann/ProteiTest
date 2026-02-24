#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>

#include "display.hpp"
#include "hashed_values.hpp"
#include "menu_functions.hpp"
#include "parsing.hpp"
#include "settings.hpp"
#include "static_containers.hpp"

int main(int argc, char* argv[])
{

  std::unordered_map<size_t, std::string_view> cl_args{
      {hashed::kAddrHash, "127.0.0.1"}, {hashed::kPortHash, "5555"},
      {hashed::kRoleHash, "Client"},    {hashed::kIndexHash, "0"},
      {hashed::kLibHash, "Lib1"},
  };
  parseClArgs(cl_args, argv, argc);

  Settings command_line_options{};
  if (!parseAddr(cl_args.at(hashed::kAddrHash), command_line_options)) {
    std::cout << "Invalid IP address\n";
    return 0;
  }

  if (!parsePort(cl_args.at(hashed::kPortHash), command_line_options)) {
    std::cout << "Invalid port\n";
    return 0;
  }

  if (!parseIndex(cl_args.at(hashed::kIndexHash), command_line_options)) {
    std::cout << "Invalid index\n";
    return 0;
  }

  command_line_options.setLibName(cl_args.at(hashed::kLibHash));
  command_line_options.setRole(cl_args.at(hashed::kRoleHash));

  displayMenu();

  PolymorpicVector<kVectorDimensionsAmount> task_vector{};

  //if i have reference to static object, as I hashed::knowm there won't be any additional
  //calls to check if static object is initialized
  const std::unordered_map<size_t, static_containers::MenuOptions>&
      menu_options = static_containers::getMenuOptions();

  while (true) {
    std::string text_option;

    std::cout << "Your command: ";

    std::cin >> text_option;
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = menu_options.contains(input_hash);
    auto picked_option = is_correct_option
                             ? menu_options.at(input_hash)
                             : static_containers::MenuOptions::WrongOption;
    switch (picked_option) {
      case static_containers::MenuOptions::ChangeRole:
        changeRole(command_line_options);

        displayMenu();
        clearCinBuffer();
        break;

      case static_containers::MenuOptions::ChangeType:
        changeType(command_line_options);

        displayMenu();
        clearCinBuffer();
        break;

      case static_containers::MenuOptions::PrintSettings:
        std::cout << '\n' << command_line_options << '\n';
        break;

      case static_containers::MenuOptions::PrintCurrentVector:
        printVector(task_vector);
        break;

      case static_containers::MenuOptions::EnterVector:
        enterVector(task_vector, command_line_options);

        displayMenu();
        clearCinBuffer();
        break;

      case static_containers::MenuOptions::QuitProgram:
        return 0;

      default:
        std::cout << "Wrong menu option, try again\n";
    }
  }
}

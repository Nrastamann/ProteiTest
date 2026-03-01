#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

#include "display.hpp"
#include "hashed_values.hpp"
#include "menu.hpp"
#include "menu_functions.hpp"
#include "parsing.hpp"
#include "settings.hpp"
#include "static_containers.hpp"

int main(int argc, char* argv[])
{

  std::unordered_map<size_t, std::string_view> cl_args{
      {hashed::kAddrHash, "127.0.0.1"}, {hashed::kPortHash, "5555"},
      {hashed::kRoleHash, "Client"},    {hashed::kIndexHash, "0"},
      {hashed::kLibHash, ""},
  };

  switch (parsing_protei::parseClArgs(cl_args, argv, argc)) {
    case parsing_protei::ParseResult::WRONG_FLAG:
      std::cerr << "Wrong flag passed\n";
      return 1;
      break;
    case parsing_protei::ParseResult::NO_ARGUMENT:
      std::cerr << "Flag with argument passed without one\n";
      return 1;
    default:
      break;
  }

  Settings command_line_options{};
  if (!parsing_protei::parseAddr(cl_args.at(hashed::kAddrHash),
                                 command_line_options)) {
    std::cout << "Invalid IP address\n";
    return 0;
  }

  if (!parsing_protei::parsePort(cl_args.at(hashed::kPortHash),
                                 command_line_options)) {
    std::cout << "Invalid port\n";
    return 0;
  }

  if (!parsing_protei::parseIndex(cl_args.at(hashed::kIndexHash),
                                  command_line_options)) {
    std::cout << "Invalid index\n";
    return 0;
  }

  command_line_options.setLibName(cl_args.at(hashed::kLibHash));
  command_line_options.setRole(cl_args.at(hashed::kRoleHash));

  ui_protei::displayMenu();

  PolymorphicVector<kVectorDimensionsAmount> task_vector{};

  //if i have reference to static object, as I hashed::knowm there won't be any additional
  //calls to check if static object is initialized
  const std::unordered_map<size_t, static_containers::MenuOptions>&
      menu_options = static_containers::getMenuOptions();

  Menu menu;

  FunctionArgs arguments{command_line_options, task_vector};

  while (!command_line_options.cgetShouldClose()) {
    std::string text_option;

    std::cout << "Your command: ";

    std::cin >> text_option;
    std::ranges::transform(text_option, text_option.begin(), ::tolower);

    size_t input_hash = std::hash<std::string_view>{}(text_option);

    bool is_correct_option = menu_options.contains(input_hash);

    static_containers::MenuOptions picked_option =
        is_correct_option ? menu_options.at(input_hash)
                          : static_containers::MenuOptions::WrongOption;

    menu.callFunction(picked_option, 0, 0, arguments);
  }
  ui_protei::clearScreen();
}

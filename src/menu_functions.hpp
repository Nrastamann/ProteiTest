#pragma once
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <variant>
#include "data_pool.hpp"
#include "settings.hpp"
#include "static_containers.hpp"
#include "utility.hpp"
struct NonConstTag {};
namespace static_containers {
const std::unordered_map<static_containers::EnumTypes, any_type>&
getDefaultValues();
}  // namespace static_containers

namespace menu_functions_protei {
void changeType(AppSettings& settings);
void changeName(AppSettings& settings);

void enterVector(DataPool& vector, AppSettings const& settings);

inline void quit(AppSettings& settings)
{
  settings.setShouldClose();
}

inline void emptyQueue(DataPool& data_pool, NonConstTag)
{
  while (data_pool.size() > 0) {
    auto vec = data_pool.front()._vec;
    for (const auto& i : vec) {
      std::visit(Visitor{[](auto const& variant_val) {
                           std::cout << variant_val << ' ';
                         },
                         [](int8_t value) { std::cout << +value << ' '; },
                         [](uint8_t value) { std::cout << +value << ' '; }},
                 i);
    }
    std::cout << " - "
              << static_containers::getImplementedTypes().at(
                     data_pool.front()._type_hash)
              << '\n';

    data_pool.pop();
  }
  std::cout << "Queue is empty\n";
}

inline void printCurrentAppSettings(AppSettings& settings)
{
  ui_protei::printAppSettings(settings);
}

inline void printVector(const DataPool& arr)
{
  for (const auto& i : arr.front()._vec) {
    std::visit(Visitor{[](auto const& variant_val) {
                         std::cout << variant_val << ' ';
                       },
                       [](int8_t value) { std::cout << +value << ' '; },
                       [](uint8_t value) { std::cout << +value << ' '; }},
               i);
  }
  std::cout << '\n';
}
inline void wrongOption()
{
  std::cout << "Wrong menu option, try again\n";
}
}  // namespace menu_functions_protei

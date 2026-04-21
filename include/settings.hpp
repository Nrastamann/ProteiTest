#pragma once
#include "exchange_ue.hpp"
#include "logger.hpp"
#include "parsing.hpp"
#include "ue_context.hpp"

class AppSettings {
 public:
  explicit AppSettings(parsing::ArgHolder& arguments, ue::DeviceConfiguration& config)
      : _exchanger(config, arguments.getAddr()[0], arguments.getX())
  {
  }
  [[nodiscard]] bool cgetShouldClose() const { return _should_close; }

  void setShouldClose() { _should_close = true; }
  [[nodiscard]] ue::Exchanger& getExchanger() { return _exchanger; }

 private:
  ue::Exchanger _exchanger;
  bool _should_close{false};
};

namespace ui_protei {
void printAppSettings(AppSettings& settings);
}  // namespace ui_protei

#pragma once
#include <memory>
#include <unordered_map>
#include "mncs_mme.hpp"

namespace mncs {
class EnodeBHelper {
 public:
  void run(std::unordered_map<size_t, std::unique_ptr<mncs::BaseStation>>& _stations,
           mncs::MME& _mme);

  void transmitToEnodeb(
      std::unordered_map<size_t, std::unique_ptr<mncs::BaseStation>>& _stations);
  void transmitToMme(mncs::MME& _mme);
  void terminate() { _should_close = true; }

 private:
  bool _should_close{false};
};
}  // namespace mncs

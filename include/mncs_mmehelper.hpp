#pragma once
#include <thread>
#include "mncs_basestation.hpp"
#include "mncs_mme.hpp"
namespace mncs {

class Helper {
  void pushToMME(std::unordered_map<size_t, std::unique_ptr<BaseStation>>& ptr_to_nodes,
                 mncs::MME& mme);
  void readFromMME(std::unordered_map<size_t, std::unique_ptr<BaseStation>>& ptr_to_nodes,
                   mncs::MME& mme);

 public:
  Helper(std::unordered_map<size_t, std::unique_ptr<BaseStation>>& ptr_to_nodes, mncs::MME& mme)
  {
    std::thread tomme(&Helper::pushToMME, this, std::ref(ptr_to_nodes), std::ref(mme));
    std::thread frommme(&Helper::readFromMME, this, std::ref(ptr_to_nodes), std::ref(mme));

    tomme.detach();
    frommme.detach();
  }
  void terminate() { _should_close = true; }

 private:
  bool _should_close{false};
};
}  // namespace mncs

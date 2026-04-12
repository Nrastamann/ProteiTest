#include "transmition_helper.hpp"
#include <thread>
namespace mncs {

void EnodeBHelper::run(
    std::unordered_map<size_t, std::unique_ptr<mncs::BaseStation>>& _stations, mncs::MME& _mme)
{
  std::thread worker1 = std::thread(&EnodeBHelper::transmitToMme, std::ref(_mme));
  std::thread worker2 = std::thread(&EnodeBHelper::transmitToEnodeb, std::ref(_stations));

  worker1.detach();
  worker2.detach();
}

void EnodeBHelper::transmitToEnodeb(
    std::unordered_map<size_t, std::unique_ptr<mncs::BaseStation>>& _stations)
{
  while (!_should_close) {
    for (auto& station : _stations) {}
  }
}
void EnodeBHelper::transmitToMme(mncs::MME& _mme) {}

}  // namespace mncs

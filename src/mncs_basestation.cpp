#include "mncs_basestation.hpp"
#include <variant>
#include "utility.hpp"
namespace mncs {
void BaseStation::run()
{
  while (!_shutdown) {
    for (auto& connected : _flags) {
      if (!connected) {
        continue;
      }

      if (_read_queue.size() == 0) {
        continue;
      }
      //auto* msg = _read_queue.front();
      //need mme reference/ptr
      /*std::visit(utility::Visitor{[](auto& i) {

                 }},
                 msg);*/
    }
  }
}
int BaseStation::remove() {}
BaseStation* BaseStation::connect(UEConnection& connection)
{
  auto* lock_it = _locks.begin();
  for (auto* it = _flags.begin(); it != _flags.end();
       std::advance(it, 1), std::advance(lock_it, 1)) {

    if (*it || !lock_it->tryLock()) {
      continue;
    }
    lock_it->lock();
    *it = true;
    lock_it->unlock();
    connection.setidx(it - _flags.begin());
    return this;
  }
  return nullptr;
}

}  // namespace mncs

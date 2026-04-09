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

      if (_buffer_received.size() == 0) {
        continue;
      }
      auto* msg = _buffer_received.front();
      //need mme reference/ptr
      std::visit(utility::Visitor{[](auto& i) {

                 }},
                 msg);
    }
  }
}
}  // namespace mncs

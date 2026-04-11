#include "mncs_mme.hpp"
#include <variant>
#include "utility.hpp"
namespace mncs {
void MME::startMME() {}
void MME::pushToMME() {}
void MME::processMessages()
{
  msg_handover_type* handover_ptr{nullptr};
  msg_type* msg_ptr{nullptr};
  while (_is_on) {
    handover_ptr = _handover_queue.front();
    msg_ptr = _handover_queue.front();
    while (nullptr != handover_ptr) {

      std::visit(utility::Visitor{[](auto& msg) {}}, *handover_ptr);

      _handover_queue.pop();
      handover_ptr = _handover_queue.front();
    }
    while (/*_lock.tryLock() &&*/ nullptr != msg_ptr) {
      std::visit(utility::Visitor{[](auto& msg) {}}, *msg_ptr);

      _message_queue.pop();
      msg_ptr = _message_queue.front();
    }
  }
}
void MME::pushToHandover() {}
}  // namespace mncs

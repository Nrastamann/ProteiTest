#include "mncs_mmehelper.hpp"
#include <variant>
#include "mncs_messages.hpp"
#include "utility.hpp"
namespace mncs {
void Helper::pushToMME(std::unordered_map<size_t, std::unique_ptr<BaseStation>>& ptr_to_nodes,
                       mncs::MME& mme)
{
  while (!_should_close) {
    auto& queue_from_mme = mme.getFromMme();
    auto& handover = mme.getFromHandover();
    while (handover.front() != nullptr) {
      auto& msg = std::get<ReleaseBuffer>(*handover.front());
      ptr_to_nodes.at(msg._enodeb_id)->handoverMsgsRecv().push(msg);
    }
    while (queue_from_mme.front() != nullptr) {
      auto* msg = queue_from_mme.front();
      size_t trgt = std::visit(
          utility::Visitor{[](AuthRequest& msg) { return msg._id._enodeb_id; },
                           [](AttachAccept& msg) { return msg._id._enodeb_id; },
                           [](RemoveConnection& msg) { return msg._id._enodeb_id; },
                           [](RouteRequestEB& msg) { return msg._id._id._enodeb_id; },
                           [](StatusReport& msg) { return msg._id._id._enodeb_id; }},
          *msg);
      auto& lock = ptr_to_nodes.at(trgt)->getLockRecv();
      lock.lock();
      ptr_to_nodes.at(trgt)->mmeMsgsRecv().push(*msg);
      lock.unlock();
      queue_from_mme.pop();
    }

    for (auto& node : ptr_to_nodes) {
      auto& lock = node.second->getLockRecv();
      auto& reroute = node.second->rerouteSend();
      while (reroute.front() != nullptr) {
        auto* msg = reroute.front();
        lock.lock();
        ptr_to_nodes.at(msg->_enodeb_id)->rerouteRecv().push(msg->_data);
        lock.unlock();
        reroute.pop();
      }

      auto& enodeb = node.second->enodebSendQ();
      while (enodeb.front() != nullptr) {
        auto* msg = enodeb.front();
        size_t trgt =
            std::visit(utility::Visitor{[](Forward& msg) { return msg._enodeb_target; }}, *msg);
        lock.lock();
        ptr_to_nodes.at(trgt)->enodebRecvQ().push(*msg);
        lock.unlock();
        reroute.pop();
      }

      auto& queue_to_mme_eb = node.second->mmeMsgsSend();
      auto& queue_to_mme = mme.getToMme();

      while (queue_to_mme_eb.front() != nullptr) {
        auto* msg = queue_to_mme_eb.front();
        queue_to_mme.push(*msg);
        queue_to_mme_eb.pop();
      }

      auto& queue_to_handover = mme.getToHandover();
      auto& queue_to_handover_eb = node.second->handoverMsgsSend();

      while (queue_to_handover_eb.front() != nullptr) {
        auto* msg = queue_to_handover_eb.front();
        queue_to_handover.push(*msg);
        queue_to_handover_eb.pop();
      }
    }
  }
}
void Helper::readFromMME(XLR& xlr, mncs::MME& mme)
{
  while (!_should_close) {
    auto& fromhlr = xlr.fromhlr();
    auto& tohlr = xlr.tohlr();
    auto& mme_fromhlr = mme.getFromHLR();
    auto& mme_tohlr = mme.getToHLR();

    while (fromhlr.size() != 0) {
      auto* msg = fromhlr.front();
      mme_fromhlr.push(*msg);
      fromhlr.pop();
    }

    while (mme_tohlr.size() != 0) {
      auto* msg = mme_tohlr.front();
      tohlr.push(*msg);
      mme_tohlr.pop();
    }
  }
  //unimplemented
}
}  // namespace mncs

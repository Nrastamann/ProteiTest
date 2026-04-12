#include "mncs_basestation.hpp"
#include <chrono>
#include <functional>
#include <set>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include "mncs_messages.hpp"
#include "timer.hpp"
#include "ue_messages.hpp"
#include "utility.hpp"
namespace mncs {
void BaseStation::updateTTL()
{
  std::vector<size_t> connections_expired;
  std::set<size_t> connections_timeout;
  while (!_shut_down) {
    connections_timeout.clear();
    connections_expired.resize(0);

    auto* ttl = _update_ttl.front();
    while (ttl != nullptr) {
      _net_connection_lock.lock();
      std::visit(utility::Visitor{[this](TTLResetUE msg) {
                                    _connections.at(msg._connection_id).second.restart();
                                  },
                                  [this, &connections_timeout](TTLFree msg) {
                                    connections_timeout.erase(msg._connection_id);
                                  }},
                 *ttl);
      _net_connection_lock.unlock();

      _update_ttl.pop();
      ttl = _update_ttl.front();
    }

    _handover_buffer_lock.lock();  //bcz can cancel handover
    for (auto& element : _reroute_service) {
      if (element.second->_isExpired && !element.second->_timer.checkTimer()) {
        connections_expired.push_back(element.first);
      }
    }
    _handover_buffer_lock.unlock();

    for (auto& element : _connections) {
      if (!element.second.second.checkTimer()) {
        connections_timeout.insert(element.first);
      }
    }

    for (auto& expired : connections_expired) {
      releaseBuffer(expired);
    }

    for (const auto& connection : connections_timeout) {
      auto& connection_timeout = _connections.at(connection);
      _lock_send_queues.lock();
      _mmeMsgsSend.push(OutOfService{._tmsi = connection_timeout.first->getTmsi()});
      _lock_send_queues.unlock();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTimeTTLUpdate));
  }
}
void BaseStation::pushToConnections(mncs::UEConnection* connection)
{
  _net_connection_lock.lock();
  _connections.insert({connection->getIdx(), {connection, pr_utils::Timer(_ttl_ue)}});
  _net_connection_lock.unlock();
}

void BaseStation::markAsExpired(size_t connection_id)
{
  _handover_buffer_lock.lock();
  auto it = _reroute_service.find(connection_id);
  if (it != _reroute_service.end()) {
    it->second->_isExpired = true;
    it->second->_timer.restart();
  }
  _handover_buffer_lock.unlock();
}
void BaseStation::pushToHandoverBuffer(size_t connection_id, size_t target_enodeb)
{
  _handover_buffer_lock.lock();
  _reroute_service.insert({connection_id, std::make_unique<HandoverServiceBuffer>(
                                              kHandoverDecayMS, target_enodeb)});
  _handover_buffer_lock.unlock();
}

//NEED TO LOCK BEFORE AND UNLOCK AFTER MANUALLY, BCZ CONNECTION MAY BE FREE'D
void BaseStation::pushDataToHandoverBuffer(size_t connection_id, serviceMsg& msg, bool isSend)
{
  auto* buf = _reroute_service.at(connection_id).get();
  isSend ? buf->_send.push(msg) : buf->_recv.push(msg);
  buf->_timer.restart();
}

bool BaseStation::tryPush(serviceMsg& msg)
{
  return std::visit(
      utility::Visitor{
          [this](auto& msg) { !this->_enodeb_send_q.try_push(msg); },
          [this](EnodeBFromMME& msg) { return !this->_mmeMsgsRecv.try_push(msg); },
          [this](EnodeBToMME& msg) { return !this->_mmeMsgsSend.try_push(msg); },
          [this](EnodeBEnodeBRecv& msg) { return !this->_enodeb_recv_q.try_push(msg); },
      },  //need to check, if send/recv is work propperly
      msg);
}
bool BaseStation::contains(size_t connection_id) {}
void BaseStation::releaseBuffer(size_t connection_id)
{
  _handover_buffer_lock.lock();
  auto buffer = std::move(_reroute_service.at(connection_id));
  _reroute_service.erase(connection_id);

  auto* send = buffer->_send.front();
  while (send != nullptr) {
    _reroute_send.push(ServiceMsgWrapper{._data = *send, ._enodeb_id = buffer->_target_enodeb});
    buffer->_send.pop();
    send = buffer->_send.front();
  }

  send = buffer->_recv.front();
  while (send != nullptr) {
    _reroute_send.push(ServiceMsgWrapper{._data = *send, ._enodeb_id = buffer->_target_enodeb});
    buffer->_recv.pop();
    send = buffer->_recv.front();
  }
}

void BaseStation::cancelHandover(size_t connection_id)
{
  _handover_buffer_lock.lock();

  auto buffer = std::move(_reroute_service.at(connection_id));
  _reroute_service.erase(connection_id);
  _handover_buffer_lock.unlock();
  //need to check recv/send buffer
  while (buffer->_recv.size() + buffer->_send.size() != 0) {
    auto* send = buffer->_send.front();
    auto* recv = buffer->_recv.front();
    _lock_send_queues.lock();
    while (send != nullptr) {
      if (tryPush(*send)) {
        break;
      }
      buffer->_send.pop();
      send = buffer->_send.front();
    }
    _lock_send_queues.unlock();
    _lock_recv_queues.lock();
    while (recv != nullptr) {
      if (tryPush(*recv)) {
        break;
      }
      buffer->_recv.pop();
      recv = buffer->_recv.front();
    }
    _lock_recv_queues.unlock();
  }
}

//=========

uint64_t BaseStation::getPower(int64_t pos) const
{
  return kMaxPower - (std::abs(_x - pos) / _radius);
}
size_t BaseStation::calculateMessageID(std::string_view str, size_t msisdn)
{
  return std::hash<size_t>{}(std::hash<std::string_view>{}(str) + std::hash<size_t>{}(msisdn));
}
void BaseStation::handover()
{
  auto* recv = _handoverMsgsRecv.front();
  while (!_shut_down) {
    recv = _handoverMsgsRecv.front();
    while (recv != nullptr) {
      std::visit(utility::Visitor{
                     [this](HandoverStartInit& msg) {
                       pushToHandoverBuffer(msg._connection_id, _idx);
                       _handoverMsgsSend.push(
                           HandoverRequest{._connection_id = msg._connection_id,
                                           ._id = {._dst_id = msg._dst_id, ._src_id = _idx}});
                     },
                     [this](HandoverRequest& msg) {
                       auto* ptr = _buffer.getBuffer();
                       bool availability = ptr != _buffer.end();
                       _handoverMsgsSend.push(HandoverAck{
                           ._id = {._src_id = msg._id._dst_id, ._dst_id = msg._id._src_id},
                           ._connection_id = msg._connection_id,
                           ._isAvailable = availability,
                           ._buffer_idx = static_cast<size_t>(ptr - _buffer.begin())});
                     },
                     [this](HandoverAck& msg) {
                       if (!msg._isAvailable) {
                         cancelHandover(msg._connection_id);
                         return;
                       }
                       _handoverMsgsSend.push(Move{
                           ._id = {._src_id = msg._id._dst_id, ._dst_id = msg._id._src_id},
                           ._buffer_idx = msg._buffer_idx,
                           ._connectionToMove = _connections.at(msg._connection_id).first});
                     },
                     [this](Move& msg) {
                       auto* prev_buffer = msg._connectionToMove->getBuffer();  //get station
                       auto* current_buffer =
                           std::next(_buffer.begin(), static_cast<int64_t>(msg._buffer_idx));

                       for (auto& msg : *prev_buffer) {
                         current_buffer->insert({msg.first, msg.second});
                       }
                       _net_connection_lock.lock();
                       msg._connectionToMove->setBuffer(current_buffer);
                       msg._connectionToMove->setEnodeb(_idx);

                       _connections.insert({msg._connectionToMove->getIdx(),
                                            {msg._connectionToMove, pr_utils::Timer(_ttl_ue)}});
                       _net_connection_lock.unlock();

                       _handoverMsgsSend.push(
                           SwitchEnodeB{._connection_id = msg._connectionToMove->getIdx()});
                     },
                     [this](ReleaseBuffer& msg) {
                       _buffer.releaseBuffer(msg._prev_buffer);
                       _net_connection_lock.lock();
                       _connections.erase(msg._connection_id);
                       _net_connection_lock.unlock();
                       markAsExpired(msg._connection_id);
                     },
                     [](auto&) { std::unreachable(); }},
                 *recv);
    }
  }
}
void BaseStation::ebsend()
{
  auto* enodeb = _enodeb_recv_q.front();
  auto* mme = _mmeMsgsRecv.front();
  std::vector<size_t> connections;
  while (_shut_down) {
    while (enodeb != nullptr) {
      std::visit(utility::Visitor{[this](auto&) {}}, *enodeb);
      //      _handover_buffer_lock.lock();

      _enodeb_recv_q.pop();
      enodeb = _enodeb_recv_q.front();
    }
    while (mme != nullptr) {
      std::visit(utility::Visitor{
                     [this](AuthRequest& msg) {
                       _connections.at(msg._id._connection_id)
                           .first->pushToUE(messages::ue::AuthRequest{._tmsi = msg._tmsi});
                     },
                     [this](AttachAccept& msg) {
                       _connections.at(msg._id._connection_id)
                           .first->pushToUE(messages::ue::AttachResponse{});
                     },
                     [this](RemoveConnection& msg) {
                       _update_ttl.push(mncs::TTLFree{msg._connection_id});
                     },
                     [this](RouteRequestAnsNegative& msg) {}, [this](RouteRequestAns& msg) {},
                     [this](DeliveryReport& msg) {}},
                 *mme);
      _mmeMsgsRecv.pop();
      mme = _mmeMsgsRecv.front();
    }
    for (auto& connection : _connections) {}
  }
}
void BaseStation::ebrecv() {}
}  // namespace mncs

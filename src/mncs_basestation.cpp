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
      std::visit(
          utility::Visitor{
              [this](TTLResetUE msg) { _connections.at(msg._connection_id).second.restart(); },
              [this, &connections_timeout](TTLFree msg) {
                auto* connection = _connections.at(msg._connection_id).first;
                connections_timeout.erase(msg._connection_id);
                connection->terminate();
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

    _net_connection_lock.lock();
    for (auto& element : _connections) {
      if (!element.second.second.checkTimer()) {
        connections_timeout.insert(element.first);
      }
    }
    _net_connection_lock.unlock();

    for (auto& expired : connections_expired) {
      releaseBuffer(expired);
    }

    for (const auto& connection : connections_timeout) {
      auto& connection_timeout = _connections.at(connection);
      _lock_send_queues.lock();
      _mmeMsgsSend.push(
          OutOfService{._tmsi = connection_timeout.first->getTmsi(), ._enodeb_id = _idx});
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
  _handover_buffer_lock.lock();
  auto* buf = _reroute_service.at(connection_id).get();
  size_t target = buf->_target_enodeb;
  std::visit(
      utility::Visitor{
          [this, target](auto& msg) { std::get<Forward>(msg)._enodeb_target = target; },
          [this, target](EnodeBToMME& msg) {
            std::visit(utility::Visitor{
                           [](auto&) {},
                           [this, target](OutOfService& msg) { msg._enodeb_id = target; },
                           [this, target](SendInto& msg) { msg._id._id._enodeb_id = target; }},
                       msg);
          },
          [this, target](EnodeBFromMME& msg) {
            std::visit(
                utility::Visitor{
                    [this, target](RemoveConnection& msg) { msg._id._enodeb_id = target; },
                    [this, target](RouteRequestEB& msg) { msg._id._id._enodeb_id = target; },
                    [this, target](StatusReport& msg) { msg._id._id._enodeb_id = target; },
                    [](auto&) {}},
                msg);
          }},
      msg);
  isSend ? buf->_send.push(msg) : buf->_recv.push(msg);
  buf->_timer.restart();
  _handover_buffer_lock.unlock();
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
  _net_connection_lock.lock();
  _connections.erase(connection_id);
  _net_connection_lock.unlock();
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

                       for (auto& msg : prev_buffer->second) {
                         current_buffer->second.push_back(msg);
                       }
                       _net_connection_lock.lock();
                       msg._connectionToMove->setBuffer(current_buffer);
                       msg._connectionToMove->setEnodeb(_idx);

                       _connections.insert({msg._connectionToMove->getIdx(),
                                            {msg._connectionToMove, pr_utils::Timer(_ttl_ue)}});
                       _net_connection_lock.unlock();

                       _handoverMsgsSend.push(SwitchEnodeB{
                           ._id = {._dst_id = msg._id._src_id, ._src_id = msg._id._dst_id},
                           ._connection_id = msg._connectionToMove->getIdx(),
                           ._tmsi = msg._connectionToMove->getTmsi()});
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

static size_t getHash(std::string_view str, size_t msisdn)
{
  return std::hash<size_t>{}(std::hash<std::string_view>{}(str) + std::hash<size_t>{}(msisdn));
}

void BaseStation::ebsend()
{
  auto* enodeb = _enodeb_recv_q.front();
  auto* mme = _mmeMsgsRecv.front();
  EnodeBRecv* connection_msg = nullptr;
  std::vector<size_t> connections;
  while (_shut_down) {
    while (enodeb != nullptr) {
      std::visit(utility::Visitor{[this](auto&) {}}, *enodeb);
      //      _handover_buffer_lock.lock();

      _enodeb_recv_q.pop();
      enodeb = _enodeb_recv_q.front();
    }
    while (mme != nullptr) {
      std::visit(
          utility::Visitor{
              [this](AuthRequest& msg) {
                _connections.at(msg._id._connection_id)
                    .first->pushToUE(messages::ue::AuthRequest{._tmsi = msg._tmsi});
              },
              [this](AttachAccept& msg) {
                _connections.at(msg._id._connection_id)
                    .first->pushToUE(messages::ue::AttachResponse{});
              },
              [this](RemoveConnection& msg) {
                _update_ttl.push(mncs::TTLFree{._connection_id = msg._id._connection_id});
              },
              [this](RouteRequestEB& msg) {
                if (_reroute_service.contains(msg._id._id._connection_id)) {
                  serviceMsg msg_send = msg;
                  pushDataToHandoverBuffer(msg._id._id._connection_id, msg_send, false);
                  return;
                }
                auto& connection = _connections.at(msg._id._id._connection_id);
                if (msg._enodeb_target == _idx) {
                  connection.first->pushToUE(messages::ue::AcknowledgmentResponse{
                      ._tmsi = connection.first->getTmsi(),
                      ._message_id = msg._id._smsid,
                      ._status = messages::ue::SMSStatus::Lost});
                  return;
                };
                _lock_send_queues.lock();
                _enodeb_send_q.push(Forward{
                    ._enodeb_target = msg._enodeb_target,
                    ._msg =
                        _buffer.findBufferData(connection.first->getBuffer(), msg._id._smsid)
                            ._sms});
                _lock_send_queues.unlock();
              },
              [this](StatusReport& msg) {
                if (_reroute_service.contains(msg._id._id._connection_id)) {
                  serviceMsg msg_send = msg;
                  pushDataToHandoverBuffer(msg._id._id._connection_id, msg_send, false);
                  return;
                }
                auto& connection = _connections.at(msg._id._id._connection_id);
                connection.first->pushToUE(messages::ue::AcknowledgmentResponse{
                    ._status = messages::ue::SMSStatus::Received,
                    ._message_id = msg._id._smsid,
                    ._tmsi = connection.first->getTmsi()});
                _buffer.removeBufferData(connection.first->getBuffer(), msg._id._smsid);
              }},
          *mme);
      _mmeMsgsRecv.pop();
      mme = _mmeMsgsRecv.front();
    }
    _net_connection_lock.lock();
    for (auto& connection : _connections) {
      if (_reroute_service.contains(connection.first)) {
        continue;
      }
      connection_msg = connection.second.first->getFromUE();
      while (connection_msg != nullptr) {

        std::visit(
            utility::Visitor{
                [this, &connection](Ping) {
                  _update_ttl.push(TTLResetUE{._connection_id = connection.first});
                },
                [this](AttachRequest& msg) {
                  _lock_send_queues.lock();
                  _mmeMsgsSend.push(msg);
                  _lock_send_queues.unlock();
                },
                [this](AuthResponse& msg) {
                  _lock_send_queues.lock();
                  _mmeMsgsSend.push(msg);
                  _lock_send_queues.unlock();
                },
                [this, &connection](HandoverStart& msg) {
                  _handoverMsgsSend.push(HandoverStartInit{._dst_id = msg._dst_id,
                                                           ._connection_id = connection.first});
                },
                [this, &connection](messages::ue::SmsReqNet& msg) {
                  (connection.second.first->getBuffer())
                      ->second.push_back({._sms = msg, ._tmsi = msg._tmsi, ._flag = false});
                  _lock_send_queues.lock();
                  _mmeMsgsSend.push(SendInto{
                      ._id = {._smsid = getHash(std::string_view{msg._sms}, msg._msisdn)},
                      ._tmsi_s = msg._tmsi,
                      ._msisdn = msg._msisdn});
                  _lock_send_queues.unlock();
                },
                [this](messages::ue::AcknowledgmentRequest& msg) {
                  _lock_send_queues.lock();
                  _mmeMsgsSend.push(
                      DeliveryReport{._sms_id = msg._sms_id, ._tmsi_d = msg._tmsi_d});
                  _lock_send_queues.unlock();
                },
                [this, &connection](messages::ue::AuthResponse& msg) {
                  connection.second.first->pushToUE(msg);
                }

            },
            *connection_msg);
        connection.second.first->popFromUe();
        connection_msg = connection.second.first->getFromUE();
      }
    }
    _net_connection_lock.unlock();

    auto* reroute = _reroute_recv.front();
    while (reroute != nullptr) {
      std::visit(utility::Visitor{
                     [this](EnodeBToMME& msg) {
                       _lock_send_queues.lock();
                       _mmeMsgsSend.push(msg);
                       _lock_send_queues.unlock();
                     },
                     [this](auto& msg) {
                       _lock_recv_queues.lock();
                       _enodeb_recv_q.push(msg);
                       _lock_recv_queues.unlock();
                     },
                     [this](EnodeBEnodeBSend& msg) {
                       _lock_send_queues.lock();
                       _enodeb_send_q.push(msg);
                       _lock_send_queues.unlock();
                     },
                     [this](EnodeBFromMME& msg) {
                       _lock_recv_queues.lock();
                       _mmeMsgsRecv.push(msg);
                       _lock_recv_queues.unlock();
                     },
                 },
                 reroute->_data);

      _reroute_recv.pop();
      reroute = _reroute_recv.front();
    }
  }
}

rigtorp::SPSCQueue<ServiceMsgWrapper>& BaseStation::rerouteRecv()
{
  return _reroute_recv;
};
rigtorp::SPSCQueue<ServiceMsgWrapper>& BaseStation::rerouteSend()
{
  return _reroute_send;
};

rigtorp::SPSCQueue<EnodeBEnodeBRecv>& BaseStation::enodebRecvQ()
{
  return _enodeb_recv_q;
};
rigtorp::SPSCQueue<EnodeBEnodeBSend>& BaseStation::enodebSendQ()
{
  return _enodeb_send_q;
};

rigtorp::SPSCQueue<EnodeBFromMME>& BaseStation::mmeMsgsRecv()
{
  return _mmeMsgsRecv;
};
rigtorp::SPSCQueue<EnodeBToMME>& BaseStation::mmeMsgsSend()
{
  return _mmeMsgsSend;
};

rigtorp::SPSCQueue<HandoverMsg>& BaseStation::handoverMsgsRecv()
{
  return _handoverMsgsRecv;
};
rigtorp::SPSCQueue<HandoverMsg>& BaseStation::handoverMsgsSend()
{
  return _handoverMsgsSend;
};
utility::Spinlock& BaseStation::getLockRecv()
{
  return _lock_recv_queues;
}

}  // namespace mncs

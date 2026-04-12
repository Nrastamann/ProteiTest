#include "mncs_basestation.hpp"
#include <functional>
#include <utility>
#include <variant>
#include "mncs_messages.hpp"
#include "timer.hpp"
#include "utility.hpp"
namespace mncs {

void BaseStation::pushToConnections(mncs::UEConnection* connection)
{
  _net_connection_lock.lock();
  _connections.insert({connection->getIdx(), {connection, pr_utils::Timer(_ttl_ue)}});
  _net_connection_lock.unlock();
}

void BaseStation::markAsExpired(size_t connection_id)
{
  _handover_buffer_lock.lock();
  _reroute_service.at(connection_id)->_isExpired = true;
  _handover_buffer_lock.unlock();
}
void BaseStation::pushToHandoverBuffer(size_t connection_id, size_t target_enodeb)
{
  _handover_buffer_lock.lock();
  _reroute_service.insert(
      {connection_id, std::make_unique<HandoverServiceBuffer>(1000, target_enodeb)});
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
/*
void BaseStation::ebsend()
{
  utility::UEMessageData msg;
  while (!_shut_down) {
    auto* connection = _connect.front();
    while (connection != nullptr) {
      _connections.insert({(*connection)->getIdx(), {*connection, pr_utils::Timer(_ttl_ue)}});
      _connect.pop();
      connection = _connect.front();
    }

    auto* handover_msg = _handoverMsgsRecv.front();
    auto* message_mme = _mmeMsgsRecv.front();
    auto* enodeb_msg = _enodeb_recv_q.front();
    while (message_mme != nullptr || enodeb_msg != nullptr || handover_msg != nullptr) {
      while (handover_msg != nullptr) {
        std::visit(
            utility::Visitor{
                [this](utility::HandoverStart& msg) {
                  if (!_buffer.bufferAvailability()) {
                    _handoverMsgsSend.push(utility::HandoverRefuse{
                        ._idx_dst = _idx, ._idx_refused = msg._src->getStationId()});
                    return;
                  }
                  _lock_queues.lock();
                  _handoverMsgsSend.push(
                      utility::HandoverAck{._dst = this, ._src_idx = msg._src->getStationId()});
                  _move_connection = msg._src;
                },
                [this](utility::HandoverAck& msg) {
                  _handoverMsgsSend.push(
                      utility::Move{._src = msg._dst->_move_connection,
                                    ._dst_idx = _move_connection->getStationId()});
                },
                [this](utility::Move& msg) {
                  auto* buf = _buffer.getBuffer();
                  auto* prev_buf = msg._src->getCurrentBuffer();

                  msg._src->setCurrentBuffer(buf);
                  msg._src->setPrevBuffer(buf);

                  _connections.insert(
                      {msg._src->getIdx(), std::pair{msg._src, pr_utils::Timer(_ttl_ue)}});

                  _handoverMsgsSend.push(utility::SwitchMsg{._enodeb_idx_new = _idx,
                                                            ._tmsi_s = msg._src->getTmsi()});
                },

                [this](utility::FreeBufferMsg& msg) {
                  _lock_queues.unlock();
                  if (msg._idx_from == _idx) {
                    _buffer.releaseBuffer(msg._connection->getPrevBuffer());
                    msg._connection->setPrevBuffer(msg._connection->getCurrentBuffer());
                    _move_connection = nullptr;
                  }
                },
                [this](utility::HandoverRefuse&) { _lock_queues.unlock(); }, [](auto&) {}},
            *handover_msg);
        _handoverMsgsRecv.pop();
        handover_msg = _handoverMsgsRecv.front();
      }

      auto& msg = std::get<utility::ForwardSms>(*enodeb_msg);
      _lock_sms_buffer.lock();
      auto* it = _buffer.getBuffer();
      _lock_sms_buffer.unlock();

      it->insert({msg._sms_id, {msg._sms, msg._sms._tmsi}});

      _mmeMsgsSend.push(utility::ResetSMSTTL{._id = msg._sms._id});
      _enodeb_recv_q.pop();
      enodeb_msg = _enodeb_recv_q.front();

      while (message_mme != nullptr) {
        std::visit(
            utility::Visitor{
                [this](auto&) {}, [this](utility::AttachResponseMME& msg) {},
                [this](utility::AuthReqMMe& msg) {},
                [this](utility::SMSError& msg) {
                  msg._connection->pushToUE(
                      utility::AcknowledgmentResponse{._tmsi = msg._connection->getTmsi(),
                                                      ._status = utility::SMSStatus::Lost,
                                                      ._message_id = msg._sms_id});
                },
                [this](utility::SMSGood& msg) {
                  _enodeb_send_q.push(utility::ForwardSms{
                      ._connection = msg._connection,
                      ._sms_id = msg._sms_id,
                      ._sms = msg._connection->getCurrentBuffer()->at(msg._sms_id).first});
                },
                [this](utility::StatusReport& msg) {
                  if (_connections.contains(msg._id._connection_id)) {
                    auto connection = _connections.at(msg._id._connection_id);
                    _connections.at(msg._id._connection_id)
                        .first->pushToUE(utility::AcknowledgmentResponse{
                            ._message_id = msg._sms_id,
                            ._status = utility::SMSStatus::Received,
                            ._tmsi = connection.first->getTmsi()});
                  }
                  //remove from buffer, send to connection if idx is correct
                }},
            *message_mme);
        _mmeMsgsRecv.pop();
        message_mme = _mmeMsgsRecv.front();
      }
    }
  }

  for (auto& connection : _connections) {
    auto* msg_ue = connection.second.first->getFromUE();
    while (msg_ue != nullptr) {
      std::visit(
          /*utility::Visitor{
              [this, &connection](utility::AttachRequest& msg) {
          _mmeMsgsSend.push(
              utility::AttachMME{._enodeb_idx = _idx,
                                 ._id._connection_id = connection.first,
                                 ._id._transaction_id = connection.second.first->getImsi()});

          [this, &connection](utility::MeasurementReport& msg) {
            _handoverMsgsSend.push(utility::HandoverStart{._src = connection.second.first,
                                                          ._dst_idx = msg._enodeb_idx});
          },
              [this, &connection](utility::AuthResponse& msg) {
                _mmeMsgsSend.push(utility::AuthRespMME{
                    ._tmsi = msg._tmsi, ._imsi = connection.second.first->getImsi()});
              },
              [this, &connection](utility::SmsReqNet& msg) {
                connection.second.first->getCurrentBuffer()->insert(
                    {msg._smsid, {msg, msg._tmsi}});
                _mmeMsgsSend.push(utility::SendInto{._enodeb_idx = _idx,
                                                    ._connection = connection.second.first,
                                                    ._tmsi_s = msg._tmsi,
                                                    ._msisdn = msg._msisdn,
                                                    ._sms_id = msg._smsid});
              },
              [this, &connection](utility::AcknowledgmentResponse& msg) {
                uint32_t transaction_id =
                    connection.second.first->getCurrentBuffer()->at(msg._message_id).second;
                _mmeMsgsSend.push(utility::DeliveryReport{._sms_id = msg._message_id,
                                                          ._tmsi_d = msg._tmsi,
                                                          ._transaction_id = transaction_id});
              },
              [this](auto&) {
              }},
          */
/*
              *msg_ue);
      msg_ue = connection.second.first->getFromUE();
      connection.second.first->popFromUe();
    }
  }
}*/
}  // namespace mncs

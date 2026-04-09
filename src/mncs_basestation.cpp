#include "mncs_basestation.hpp"
#include <cstddef>
#include <mutex>
#include <unordered_map>
#include <variant>
#include "timer.hpp"
#include "utility.hpp"
namespace mncs {

int BaseStation::remove() {}
void BaseStation::run(std::unordered_map<size_t, BaseStation>& _enodeb_list)
{
  while (!_shutdown) {
    if (auto* msg = _receiveQ.front(); msg != nullptr) {
      std::visit(
          utility::Visitor{
              [this](utility::SmsReqNet&& msg) {
                _buffer.insert({msg._msisdn, std::tuple(msg._smsid, msg._sms.data(),
                                                        pr_utils::Timer(20000))});
                _sendMME.push(utility::SMSRoute{._msisdn = msg._msisdn, ._tmsi = msg._tmsi});
              },
              [this](utility::AcknowledgmentReq&& msg) {
                _sendMME.push(utility::DeliveryReport{._sms_id = msg._smsid,
                                                      ._tmsi_id = msg._tmsi_d,
                                                      ._status = utility::SMSStatus::Received});
              },
              [this](utility::MeasurementReq&& msg) {
                size_t power = getPower(msg._x);
                _sendQ.push(utility::MeasurementResp{
                    ._imei = msg._imei, ._info{._enodeb_id = _idx, ._enodeb_power = power}});
              },

              [this](utility::MeasurementConnectReq&& msg) {
                _sendMME.push(utility::EnodeBIDSend{._id = msg._enodeb_idx});
              },

              [this](utility::AttachReq&& msg) {
                _sendMME.push(utility::EnodeBIDSend{._id = _idx});
              },
              [](auto&& msg) {},
          },
          std::move(*msg));
    }
    if (auto* msg = _receiveMME.front(); msg != nullptr) {
      std::visit(utility::Visitor{
                     [this](utility::EnodeBIDSend&& msg) {},
                     [](utility::TimeoutUE&& msg) {},
                     [](utility::ResetSmsttl&& msg) {},
                     [](utility::DeliveryReport&& msg) {},
                     [](utility::StatusReport&& msg) {},
                     [](utility::SMSRoute&& msg) {},
                     [](utility::UpdateLocation&& msg) {},
                     [](auto&& msg) {},
                 },
                 std::move(*msg));
    }
    if (auto* msg = _receiveEnodeB.front(); msg != nullptr) {
      std::visit(utility::Visitor{
                     [this, &_enodeb_list](utility::HandoverReq msg) {
                       _lock.lock();
                       _enodeb_list.at(msg._enodeb_id)
                           ._receiveEnodeB.push(utility::HandoverResp{._enodeb_id = _idx});
                     },
                     [this, &_enodeb_list](utility::HandoverResp& msg) {
                       std::lock_guard<std::mutex>(this->_lock);
                       BaseStation* enodeb_swap = &_enodeb_list.at(msg._enodeb_id);
                       while (this->_receiveEnodeB.size() != 0) {
                         enodeb_swap->_receiveEnodeB.push(*_receiveEnodeB.front());
                         _receiveEnodeB.pop();
                       }
                       while (this->_sendEnodeB.size() != 0) {
                         enodeb_swap->_sendEnodeB.push(*_sendEnodeB.front());
                         _sendEnodeB.pop();
                       }
                       while (this->_receiveMME.size() != 0) {
                         enodeb_swap->_receiveMME.push(*_receiveMME.front());
                         _receiveMME.pop();
                       }
                       while (this->_sendMME.size() != 0) {
                         enodeb_swap->_sendMME.push(*_sendMME.front());
                         _sendMME.pop();
                       }
                       while (this->_receiveQ.size() != 0) {
                         enodeb_swap->_receiveQ.push(*_receiveQ.front());
                         _receiveQ.pop();
                       }
                       while (this->_sendQ.size() != 0) {
                         enodeb_swap->_sendQ.push(*_sendQ.front());
                         _sendQ.pop();
                       }
                       while (_buffer.size() != 0) {
                         enodeb_swap->_buffer.push_back(_buffer.front());
                         _buffer.erase(_buffer.begin());
                       }
                     },
                     [this, &_enodeb_list](utility::ReleaseBuffer) { _lock.unlock(); },
                     [this, &_enodeb_list](utility::SmsReqNet& msg) {

                     }

                 },
                 std::move(*msg));
    }
  }
}

}  // namespace mncs

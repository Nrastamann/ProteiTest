#include "mncs_mme.hpp"
#include <chrono>
#include <functional>
#include <thread>
#include <utility>
#include <variant>
#include "mncs_messages.hpp"
#include "timer.hpp"
#include "utility.hpp"
namespace mncs {
void MME::handover()
{
  auto* msg = _handover_queue.front();
  while (_is_on) {
    while (msg != nullptr) {
      auto& switch_msg = std::get<SwitchEnodeB>(*msg);
      _handover_ans.push(ReleaseBuffer{._enodeb_id = switch_msg._id._dst_id,
                                       ._prev_buffer = switch_msg._prev_buffer,
                                       ._connection_id = switch_msg._connection_id});

      msg = _handover_queue.front();
      _handover_queue.pop();
      _attachstate.at(switch_msg._tmsi)._connection_id = switch_msg._id._src_id;
      auto it = _smsstate.find(switch_msg._tmsi);

      if (it != _smsstate.end()) {
        for (auto& msg : it->second) {
          msg.first._enodeb_t = switch_msg._id._src_id;
        }
      }

      auto it_s = _source_sms.find(switch_msg._connection_id);

      if (it_s == _source_sms.end()) {
        return;
      }

      for (auto& idx : it_s->second) {
        _smsstate.at(idx.first).at(idx.second).first._enodeb_s = switch_msg._id._src_id;
      }
    }
  }
}
static uint64_t genTmsi(uint64_t msisdn)
{
  return std::hash<uint32_t>{}(msisdn);
}
void MME::process()
{
  while (_is_on) {
    auto* msg = _from_mme.front();
    auto* from_hlr = _from_hlr.front();
    std::visit(
        utility::Visitor{
            [](auto&) { std::unreachable(); },
            [this](AttachRequest& msg) {
              uint32_t tmsi = genTmsi(msg._data._msisdn);
              _attachstate.insert(
                  {tmsi, AttachState{._imsi = msg._data._imsi,
                                     ._imei = msg._data._imei,
                                     ._msisdn = msg._data._msisdn,
                                     ._enodeb_id = msg._id._enodeb_id,
                                     ._connection_id = msg._id._connection_id}});
              _to_hlr.push(AuthInfoRequest{._tmsi = tmsi, ._imsi = msg._data._imsi});
            },
            [this](AuthResponse& msg) {
              auto& state = _attachstate.at(msg._tmsi);

              _to_hlr.push(LocationUpdateReq{._enodeb_id = state._enodeb_id,
                                             ._msisdn = state._msisdn,
                                             ._imsi = state._imsi,
                                             ._mme_id = _mme_id});
            },
            [this](OutOfService& msg) {
              auto& state = _attachstate.at(msg._tmsi);

              _to_hlr.push(CancelLocationReq{._msisdn = state._msisdn});
            },
            [this](SendInto& msg) {
              _smsstate[msg._msisdn].push_back({{._enodeb_s = msg._id._id._enodeb_id,
                                                 ._connection_id = msg._id._id._enodeb_id,
                                                 ._sms_id = msg._id._smsid},
                                                pr_utils::Timer(kSmsttl)});
              _to_hlr.push(RouteRequest{._msisdn_dst = msg._msisdn, ._tmsi_s = msg._tmsi_s});
            },
            [this](ResetSMSTTL& msg) {
              auto it = _smsstate.find(msg._tmsi_d);
              if (it == _smsstate.end()) {
                return;
              }
              for (auto& sms : it->second) {
                if (sms.first._sms_id == msg._sms_id) {
                  sms.second.restart();
                }
              }
            },
            [this](DeliveryReport& msg) {
              auto it = _smsstate.find(msg._tmsi_d);
              if (it == _smsstate.end()) {
                return;
              }
              SMSState state{};
              size_t i = 0;
              for (auto& sms : it->second) {
                if (sms.first._sms_id == msg._sms_id) {
                  state = sms.first;
                  break;
                }
                ++i;
              }
              _from_mme.push(
                  StatusReport{._id = {._id = {._connection_id = state._connection_id,
                                               ._enodeb_id = state._enodeb_s},
                                       ._smsid = state._sms_id}});

              _smsstate.erase(it->first);
              for (auto& el : _source_sms) {
                for (auto& j : el.second) {
                  if (it->first == j.first && j.second > i) {
                    j.second--;
                  }
                }
              }
            },
        },
        *msg);

    auto* hlr_ans = _from_hlr.front();
    while (hlr_ans != nullptr) {
      std::visit(
          utility::Visitor{
              [this](AuthInfoResponse& msg) {
                auto& state = _attachstate[msg._tmsi];
                _from_mme.push(AuthRequest{._id = {._connection_id = state._connection_id,
                                                   ._enodeb_id = state._enodeb_id},
                                           ._tmsi = msg._tmsi});
              },
              [this](LocationUpdateAns& msg) {
                auto& state = _attachstate[msg._tmsi];

                _from_mme.push(AttachAccept{._id{._enodeb_id = state._enodeb_id,
                                                 ._connection_id = state._connection_id}});
              },
              [this](CancelLocationResp& msg) {
                auto& state = _attachstate[msg._tmsi];

                _from_mme.push(RemoveConnection{._id = {._connection_id = state._connection_id,
                                                        ._enodeb_id = state._enodeb_id}});
                _attachstate.erase(msg._tmsi);
              },
              [this](RouteRequestAns& msg) {
                size_t counter = 0;
                SMSState copy{};
                auto& state_now = _smsstate[msg._msisdn_dst];
                for (auto& i : state_now) {
                  if (i.first._sms_id == msg._connection_id) {
                    copy = i.first;

                    break;
                  }
                  counter++;
                }
                state_now.erase(state_now.begin() + static_cast<int64_t>(counter));

                if (!msg._state) {
                  _from_mme.push(
                      RouteRequestEB{._enodeb_target = copy._enodeb_s,
                                     ._id = {._smsid = copy._sms_id,
                                             ._id = {._connection_id = copy._connection_id,
                                                     ._enodeb_id = copy._enodeb_s}}});
                  return;
                }
                if (!_source_sms.contains(copy._connection_id)) {
                  _source_sms.insert({copy._connection_id, {}});
                }
                _source_sms.at(copy._connection_id)
                    .emplace_back(msg._tmsi_d, _source_sms.at(copy._connection_id).size());
                if (!_smsstate.contains(msg._tmsi_d)) {
                  _smsstate.insert({msg._tmsi_d, {}});
                }
                _smsstate.at(msg._tmsi_d)
                    .emplace_back(SMSState{._connection_id = msg._connection_id,
                                           ._sms_id = copy._sms_id,
                                           ._enodeb_s = copy._enodeb_s,
                                           ._enodeb_t = msg._enodeb_target},
                                  pr_utils::Timer(kSmsttl));
                _from_mme.push(
                    RouteRequestEB{._enodeb_target = msg._enodeb_target,
                                   ._id = {._smsid = copy._sms_id,
                                           ._id = {._connection_id = copy._connection_id,
                                                   ._enodeb_id = copy._enodeb_s}}});
              }},
          *hlr_ans);

      _from_hlr.pop();
      hlr_ans = _from_hlr.front();
    }

    std::vector<std::pair<idx, tmsi>> states;
    for (auto& it : _smsstate) {
      size_t c = 0;
      for (auto& sms : it.second) {
        if (!sms.second.checkTimer()) {
          states.emplace_back(c, it.first);
        }
      }
    }
    for (auto& state : states) {
      for (auto& it : _source_sms) {
        for (auto& jt : it.second) {
          if (jt.first == state.second && jt.second > state.first) {
            jt.second--;
          }
        }
      }
    }
  }
}
void MME::terminate()
{
  while (_is_on) {
    std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTimeForTerminate));
    if (!_start_ticking && _counter == 0) {
      _start_ticking = true;
      _ttlepc.restart();
    }

    if (_start_ticking && _counter > 0) {
      _start_ticking = false;
    }

    if (_start_ticking && !_ttlepc.checkTimer()) {
      _is_on = false;
    }
  }
}
}  // namespace mncs

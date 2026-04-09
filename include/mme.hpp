#pragma once
#include <chrono>
#include <cstdint>
#include <functional>
#include <variant>
#include <vector>
#include "mncs_basestation.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"
#include "xlr.hpp"
namespace mncs {
class MME {
  static constexpr size_t kMMELength{15};

 public:
  uint32_t getTMSI(uint64_t imsi) { return static_cast<uint32_t>(std::hash<uint64_t>{}(imsi)); }
  void run(XLR& xlr)
  {
    while (!_ttlepc.checkTimer()) {
      auto* msg = _receivedMME.front();
      while (msg != nullptr) {
        std::visit(
            utility::Visitor{[&xlr, this](utility::SMSRoute& msg) {
                               auto res = xlr.getTmsi(msg._msisdn);
                               if (res.first == 0 && res.second == 0) {
                                 _base_stations.at(msg._idx_sender)
                                     ._receiveMME.push((utility::TimeoutSms{}));
                               }
                               _base_stations.at(msg._idx_sender)
                                   ._receiveMME.push(
                                       {utility::RouteSMSMMe{._id_enodeb = res.second}});
                             },
                             [](utility::DeliveryReport&& msg) {

                             },
                             [](utility::EnodeBIDSend&& msg) {

                             },
                             [](utility::AuthReq&& msg) {}, [](utility::TTLOS&& msg) {},
                             [](utility::AttachReq&& msg) {}, [](utility::SwitchReq&& msg) {}},
            *msg);
        _receivedMME.pop();
        msg = _receivedMME.front();
      }
    }
  }
  std::unordered_map<size_t, BaseStation> _base_stations;
  std::vector<pr_utils::Timer> _connectionTTL;
  std::vector<pr_utils::Timer> _smsTTL;
  rigtorp::SPSCQueue<utility::MMEMsg> _receivedMME{kMMELength};

 private:
  pr_utils::Timer _ttlepc{std::chrono::seconds(12000).count()};
  uint64_t _ttl;
};
}  // namespace mncs

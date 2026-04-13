#pragma once
#include <cstdint>
#include <set>
#include <unordered_map>
#include <variant>
#include "mncs_messages.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"
namespace mncs {
class MME {
  static constexpr size_t kDefaultTtl{120000};
  static constexpr size_t kMMELength{15};
  static constexpr size_t kHandoverQueueLength{15};
  static constexpr size_t kSleepTimeForTerminate{5000};

  using tmsi = uint32_t;
  using connection_id = size_t;
  using idx = size_t;

  struct SMSState {
    size_t _sms_id;
    size_t _enodeb_s;
    size_t _enodeb_t;
    size_t _connection_id;
  };
  struct AttachState {
    uint64_t _imsi;
    uint64_t _imei;
    uint64_t _msisdn;
    size_t _enodeb_id;
    size_t _connection_id;
  };

 public:
  MME(uint64_t ttl, size_t mme_id) : _ttl(ttl), _mme_id(mme_id)
  {
    auto handover_task = std::thread(&MME::handover, this);
    auto process_task = std::thread(&MME::process, this);
    auto ttl_counter = std::thread(&MME::terminate, this);

    ttl_counter.detach();
    handover_task.detach();
    process_task.detach();
  }

  void terminate();
  void handover();
  void process();

 private:
  rigtorp::SPSCQueue<EnodeBToMME> _to_mme{kMMELength};
  rigtorp::SPSCQueue<EnodeBFromMME> _from_mme{kMMELength};

  rigtorp::SPSCQueue<ToHLR> _to_hlr{kMMELength};
  rigtorp::SPSCQueue<FromHLR> _from_hlr{kMMELength};

  rigtorp::SPSCQueue<HandoverMsg> _handover_queue{kHandoverQueueLength};
  rigtorp::SPSCQueue<HandoverMsg> _handover_ans{kHandoverQueueLength};

  std::unordered_map<tmsi, std::vector<std::pair<SMSState, pr_utils::Timer>>> _smsstate;
  std::unordered_map<tmsi, AttachState> _attachstate;
  std::unordered_map<connection_id, std::vector<std::pair<tmsi, idx>>> _source_sms;

  pr_utils::Timer _ttlepc{kDefaultTtl};
  uint64_t _ttl;
  uint64_t _mme_id;
  static constexpr size_t kSmsttl{15000};
  size_t _counter{0};
  bool _start_ticking{false};
  bool _is_on{true};
  //std::vector<int> sockets connections to other mme
};
}  // namespace mncs

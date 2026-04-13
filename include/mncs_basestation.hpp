#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include "mncs_ueconnection.hpp"
#include "rigtorp/SPSCQueue.h"
#include "timer.hpp"
#include "utility.hpp"

namespace mncs {
class UEConnection;
class BaseStation {
  static constexpr size_t kSleepTimeTTLUpdate{50};
  static constexpr size_t kTtlQueueLength{64};
  static constexpr uint64_t kMaxPower{100};
  static constexpr size_t kQueueLength{6};
  static constexpr size_t kHandoverDecayMS{2000};
  using connection_id = uint64_t;

 public:
  void ebsend();
  void handover();
  void updateTTL();

  BaseStation(int64_t x, int64_t radius, uint64_t ttl_ue)
      : _x(x), _radius(radius), _ttl_ue(ttl_ue)
  {
    if (radius == 0) {
      _radius = 1;
    }
    std::thread handover_task(&BaseStation::handover, this);
    std::thread recv_task(&BaseStation::ebsend, this);
    std::thread ttler(&BaseStation::updateTTL, this);

    handover_task.detach();
    recv_task.detach();
    ttler.detach();
  }

  [[nodiscard]] uint64_t getPower(int64_t pos) const;
  [[nodiscard]] static size_t calculateMessageID(std::string_view str, size_t msisdn);
  [[nodiscard]] size_t getIdx() const { return _idx; }
  void pushToConnections(UEConnection* connection);

  rigtorp::SPSCQueue<ServiceMsgWrapper>& rerouteRecv();
  rigtorp::SPSCQueue<ServiceMsgWrapper>& rerouteSend();

  rigtorp::SPSCQueue<EnodeBEnodeBRecv>& enodebRecvQ();
  rigtorp::SPSCQueue<EnodeBEnodeBSend>& enodebSendQ();

  rigtorp::SPSCQueue<EnodeBFromMME>& mmeMsgsRecv();
  rigtorp::SPSCQueue<EnodeBToMME>& mmeMsgsSend();

  rigtorp::SPSCQueue<HandoverMsg>& handoverMsgsRecv();
  rigtorp::SPSCQueue<HandoverMsg>& handoverMsgsSend();
  utility::Spinlock& getLockRecv();

 private:
  void markAsExpired(size_t connection_id);
  void pushToHandoverBuffer(size_t connection_id, size_t target_enodeb);
  void pushDataToHandoverBuffer(size_t connection_id, serviceMsg& msg, bool isSend);
  bool tryPush(serviceMsg& msg);
  void releaseBuffer(size_t connection_id);
  void cancelHandover(size_t connection_id);
  bool contains(size_t connection_id);
  alignas(utility::kCacheLength)::utility::Spinlock _lock_send_queues;
  alignas(utility::kCacheLength)::utility::Spinlock _lock_recv_queues;
  alignas(utility::kCacheLength)::utility::Spinlock _handover_buffer_lock;
  alignas(utility::kCacheLength)::utility::Spinlock _net_connection_lock;

  //reroute them to these queue, helper will push them to specific queues in basestation
  struct HandoverServiceBuffer {
   private:
    static constexpr size_t kBufferLength{6};

   public:
    explicit HandoverServiceBuffer(uint64_t ttl, size_t target_enodeb)
        : _timer(ttl), _target_enodeb(target_enodeb)
    {
    }
    rigtorp::SPSCQueue<serviceMsg> _send{kBufferLength};
    rigtorp::SPSCQueue<serviceMsg> _recv{kBufferLength};
    pr_utils::Timer _timer;
    size_t _target_enodeb;
    bool _isExpired{false};
  };
  std::unordered_map<size_t, std::unique_ptr<HandoverServiceBuffer>> _reroute_service;
  utility::default_buffer _buffer;

  rigtorp::SPSCQueue<TTLMsg> _update_ttl{kTtlQueueLength};

  rigtorp::SPSCQueue<ServiceMsgWrapper> _reroute_recv{4};
  rigtorp::SPSCQueue<ServiceMsgWrapper> _reroute_send{4};

  rigtorp::SPSCQueue<EnodeBEnodeBRecv> _enodeb_recv_q{4};
  rigtorp::SPSCQueue<EnodeBEnodeBSend> _enodeb_send_q{4};

  rigtorp::SPSCQueue<EnodeBFromMME> _mmeMsgsRecv{4};
  rigtorp::SPSCQueue<EnodeBToMME> _mmeMsgsSend{4};

  rigtorp::SPSCQueue<HandoverMsg> _handoverMsgsRecv{2};
  rigtorp::SPSCQueue<HandoverMsg> _handoverMsgsSend{2};

  std::unordered_map<connection_id, std::pair<UEConnection*, pr_utils::Timer>> _connections;

  size_t _idx{};

  uint64_t _ttl_ue;
  int64_t _x;
  int64_t _radius;
  bool _shut_down{false};
};
}  // namespace mncs

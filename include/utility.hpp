#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
namespace utility {
template <typename... Callable>
struct Visitor : Callable... {
  using Callable::operator()...;
};

inline constexpr size_t kThreadNum{8};
inline constexpr size_t kCacheLength = {std::hardware_destructive_interference_size};
static constexpr size_t kMaxSMSLen{480};  //max message send len equals 512 byte

//sms req/send to send across network
struct SmsReqNet {
  uint32_t _tmsi;
  uint64_t _msisdn;
  size_t _smsid;
  std::array<char, kMaxSMSLen> _sms;
};

enum class MessageFlag : uint8_t {
  SMSSend,        //Send/receive text
  SMSStatus,      //send/receive Status
  RangeReq,       //Request enodeb power
  RangeResp,      //respone enodeb power
  AttachReq,      //Attach req
  AttachResp,     //Attach resp
  AuthReq,        //Auth req
  AuthResp,       //AuthResp
  Reconnect,      //reconnect to more powerfull enodeb
  Configuration,  //Configuration resp
  HandoverDone,
  HandoverReq,
  HandoverResp,
  MMETimeout,
  DeliveryReport,
  EnodeBID,
  SmsRoute,
  TimeoutUE,
  BufferRelease,
  TTLReset
};

enum class SMSStatus : uint8_t {
  Lost,
  Waiting,
  Received,
};
struct SmsUe {
  uint64_t _msisdn;
  std::string _sms;
};
//sms status
struct AcknowledgmentResp {
  uint32_t _tmsi;
  size_t _message_id;
  SMSStatus _status;
};

//tmsi_d status to mme
struct AcknowledgmentReq {
  size_t _smsid;
  uint32_t _tmsi_d;
};

//measurement for enodeb power send
struct MeasurementReq {
  uint64_t _imei;
  int64_t _x;
};

struct EnodeBInfo {
  uint64_t _enodeb_id;
  uint64_t _enodeb_power;
};

//result off measurementreq
//for future use
struct MeasurementResp {
  uint64_t _imei;
  EnodeBInfo _info;
};

//picked enodeb connect_to
struct MeasurementConnectReq {
  uint64_t _enodeb_idx;
};
enum class EnodeBStatus : uint8_t { CONNECT, DISCONNECT };
//config message
struct ConfigResp {
  uint64_t _imei;
  uint64_t _ttl;
  EnodeBStatus _status;
};

//config message
struct AttachReq {
  uint64_t _imei;
  uint64_t _imsi;
  uint64_t _msisdn;
  uint64_t _enodeb_number;
};

//tmsi
struct AttachResponse {
  uint32_t _tmsi;
};

//config message
struct AuthReq {
  uint64_t _imei;
  uint32_t _tmsi;
};
//attached done, correctly
struct AuthResp {
  EnodeBStatus _status;
};

struct SMSRoute {
  uint64_t _idx_sender;
  uint64_t _msisdn;
  uint32_t _tmsi;
};
//from enode to mme
struct EnodeBIDSend {
  size_t _id;
};

struct DeliveryReport {
  uint64_t _sms_id;
  uint32_t _tmsi_id;
  SMSStatus _status;
};

struct HandoverReq {
  size_t _enodeb_id;  //handover to
};

struct HandoverResp {  //handover to
  size_t _enodeb_id;
};

//switch request
struct SwitchReq {
  uint64_t _idx;
};

struct HandoverDone {};

struct TimeoutUE {
  uint32_t _tmsi;
};
struct ReleaseNodeBReq {};
struct CloseResp {};
struct ReleaseBuffer {};

struct TimeoutSms {};
struct ResetSmsttl {
  uint64_t _sms_id;
};

inline constexpr size_t kMsgDataSize{sizeof(SmsReqNet)};
struct StatusReport {};
struct RouteSMSMMe {
  size_t _id_enodeb;
};
struct UpdateLocation {};
struct TTLOS {
  uint64_t tmsi;
};
using ENodeBMMESend = std::variant<SwitchReq, EnodeBIDSend, TimeoutUE, ResetSmsttl,
                                   DeliveryReport, StatusReport, SMSRoute, UpdateLocation>;
using ENodeBMMERecv = std::variant<ReleaseBuffer, CloseResp, ReleaseNodeBReq, TimeoutSms,
                                   StatusReport, RouteSMSMMe>;

using ENodeBEnodeB = std::variant<HandoverReq, HandoverResp, ReleaseBuffer, SmsReqNet>;

using UEMessageData =
    std::variant<SmsReqNet, AcknowledgmentResp, AcknowledgmentReq, MeasurementReq,
                 MeasurementResp, MeasurementConnectReq, AttachReq, AttachResponse, AuthReq,
                 AuthResp, ConfigResp, std::array<char, kMsgDataSize>>;
using MMEMsg =
    std::variant<SMSRoute, DeliveryReport, EnodeBIDSend, AuthReq, TTLOS, AttachReq, SwitchReq>;

struct UEMessage {
  MessageFlag _msg_type;
  UEMessageData _data;
};

inline constexpr size_t kMsgSize{sizeof(UEMessage)};
};  // namespace utility

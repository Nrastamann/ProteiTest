#pragma once
#include <cstddef>
#include <cstdint>
#include <variant>

#include "mncs_ueconnection.hpp"
#include "utility.hpp"

namespace mncs {
class UEConnection;

struct TTLResetUE {
  size_t _connection_id;
};
struct TTLFree {
  size_t _connection_id;
};

using TTLMsg = std::variant<TTLResetUE, TTLFree>;

struct ID {
  uint64_t _connection_id;
  uint64_t _enodeb_id;
};
//mme attach request to mme
struct AttachRequest {
  messages::ue::AttachRequest _data;
  ID _id;
};

//to hlr auth request
struct AuthInfoRequest {
  uint64_t _imsi;
  uint32_t _tmsi;
};

//hlr to mme auth request
struct AuthInfoResponse {
  uint64_t _imsi;
  uint32_t _tmsi;
};

//from mme auth request
struct AuthRequest {
  ID _id;
  uint32_t _tmsi;
};

//to mme auth response
struct AuthResponse {
  uint32_t _tmsi;
};
//mme to hlr update
struct LocationUpdateReq {
  uint64_t _imsi;
  uint64_t _msisdn;
  uint64_t _mme_id;
  size_t _enodeb_id;
};
//hlr to mme
struct LocationUpdateAns {
  uint32_t _tmsi;
};
//from mme to enode
struct AttachAccept {
  ID _id;
};
struct OutOfService {
  size_t _enodeb_id;
  uint32_t _tmsi;
};
struct CancelLocationReq {
  uint64_t _msisdn;
};
struct CancelLocationResp {
  uint64_t _tmsi;
};

struct RemoveConnection {
  ID _id;
};
//HANDOVER
struct EnodeBID {
  size_t _dst_id;
  size_t _src_id;
};
struct ConnectToEnodeB {
  ::mncs::UEConnection* _connection_to_push;
};
//Measurement report for other baseStation
struct HandoverStart {
  size_t _dst_id;
};
struct HandoverStartInit {
  size_t _dst_id;
  size_t _connection_id;
};
struct Ping {};
struct HandoverRequest {
  EnodeBID _id;
  size_t _connection_id;
};

struct HandoverAck {
  EnodeBID _id;
  size_t _connection_id;
  size_t _buffer_idx;
  bool _isAvailable;
};
struct Move {
  ::mncs::UEConnection* _connectionToMove;
  EnodeBID _id;
  size_t _buffer_idx;
};
struct SwitchEnodeB {
  size_t _connection_id;
  utility::default_buffer::iterator _prev_buffer;
  EnodeBID _id;
  uint32_t _tmsi;
};
struct ReleaseBuffer {
  size_t _connection_id;
  size_t _enodeb_id;
  utility::default_buffer::iterator _prev_buffer;
};
//SMS
struct SmsId {
  size_t _smsid;
  ID _id;
};

struct SendInto {
  uint32_t _tmsi_s;
  uint64_t _msisdn;
  SmsId _id;
};

struct RouteRequest {
  uint64_t _msisdn_dst;
  uint32_t _tmsi_s;
  size_t _connection_id;
};

struct RouteRequestAns {
  uint64_t _msisdn_dst;
  size_t _connection_id;
  size_t _enodeb_target;
  uint32_t _tmsi_d;
  uint32_t _tmsi_s;
  bool _state;
};
struct RouteRequestEB {
  SmsId _id;
  size_t _enodeb_target;
};

struct Forward {
  messages::ue::SmsReqNet _msg;
  size_t _enodeb_target;
};
struct ResetSMSTTL {
  size_t _tmsi_d;
  size_t _sms_id;
};

struct DeliveryReport {
  uint64_t _sms_id;
  uint32_t _tmsi_d;
};
struct StatusReport {
  SmsId _id;
};

using HandoverMsg = std::variant<HandoverStartInit, HandoverRequest, HandoverAck, Move,
                                 SwitchEnodeB, ReleaseBuffer>;

using EnodeBRecv =
    std::variant<AttachRequest, AuthResponse, HandoverStart,
                 Ping,  //removed ConnectToEnodeB bcz it should be pushed to enodeb
                 messages::ue::SmsReqNet, messages::ue::AcknowledgmentRequest,
                 messages::ue::AuthResponse>;  //auth response to error signaling
//also pass struct which are pushed into
//for what if scenario it better be variant
using EnodeBEnodeBSend = std::variant<Forward>;
using EnodeBEnodeBRecv = std::variant<Forward>;  //for type dispatch

using EnodeBToMME = std::variant<AttachRequest, AuthResponse, OutOfService, SendInto,
                                 ResetSMSTTL, DeliveryReport>;

using EnodeBFromMME =
    std::variant<AuthRequest, AttachAccept, RemoveConnection, RouteRequestEB, StatusReport>;

using ToHLR = std::variant<AuthInfoRequest, LocationUpdateReq, CancelLocationReq, RouteRequest>;

using FromHLR =
    std::variant<AuthInfoResponse, LocationUpdateAns, CancelLocationResp, RouteRequestAns>;
}  // namespace mncs

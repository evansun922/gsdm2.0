/*
 *  Copyright (c) 2016,
 *  It complete websocket protocol on server from [RFC6455]
 *  sunlei (sswin0922@163.com)
 */

#ifndef _WSPROTOCOL_H_
#define _WSPROTOCOL_H_

#include "platform/linuxplatform.h"
#include "buffering/iobuffer.h"

#define WS_STATE_HANDSHAKE 0
#define WS_STATE_OPEN      1

// ws header key
#define WS_HEADER_URI           "uri"
#define WS_HEADER_VERSION       "ws_version"
#define WS_HEADER_HOST          "Host"
#define WS_HEADER_UPGRADE       "Upgrade"
#define WS_HEADER_CONNECTION    "Connection"
#define WS_HEADER_WS_KEY        "Sec-WebSocket-Key"
#define WS_HEADER_ORIGIN        "Origin"
#define WS_HEADER_WS_VERSION    "Sec-WebSocket-Version"
#define WS_HEADER_WS_PROTOCOL   "Sec-WebSocket-Protocol"
#define WS_HEADER_WS_EXTENSIONS "Sec-WebSocket-Extensions"

/*
 *  1000 indicates a normal closure, meaning that the purpose for
 *  which the connection was established has been fulfilled.
 */
#define WS_STATUS_CODE_NORMAL_CLOSE 1000
/*
 *  1001 indicates that an endpoint is "going away", such as a server
 *  going down or a browser having navigated away from a page.
 */
#define WS_STATUS_CODE_GOING_AWAY 1001
/*
 *  1002 indicates that an endpoint is terminating the connection due
 *  to a protocol error.
 */
#define WS_STATUS_CODE_PROTOCOL_ERROR 1002
/*
 *  1003 indicates that an endpoint is terminating the connection
 *  because it has received a type of data it cannot accept (e.g., an
 *  endpoint that understands only text data MAY send this if it
 *  receives a binary message).
 */
#define WS_STATUS_CODE_DATA_TYPE_ERROR 1003
/*
 *  Reserved.  The specific meaning might be defined in the future.
 */
#define WS_STATUS_CODE_RESERVED 1004
/*
 *  1005 is a reserved value and MUST NOT be set as a status code in a
 *  Close control frame by an endpoint.  It is designated for use in
 *  applications expecting a status code to indicate that no status
 *  code was actually present.
 */
#define WS_STATUS_CODE_RESERVED1 1005
/*
 *  1006 is a reserved value and MUST NOT be set as a status code in a
 *  Close control frame by an endpoint.  It is designated for use in
 *  applications expecting a status code to indicate that the
 *  connection was closed abnormally, e.g., without sending or
 *  receiving a Close control frame.
 */
#define WS_STATUS_CODE_RESERVED2 1006
/*
 *  1007 indicates that an endpoint is terminating the connection
 *  because it has received data within a message that was not
 *  consistent with the type of the message (e.g., non-UTF-8 [RFC3629]
 *  data within a text message).
 */
#define WS_STATUS_CODE_TEXT_CODE_ERROR 1007
/*
 *  1008 indicates that an endpoint is terminating the connection
 *  because it has received a message that violates its policy.  This
 *  is a generic status code that can be returned when there is no
 *  other more suitable status code (e.g., 1003 or 1009) or if there
 *  is a need to hide specific details about the policy.
 */
#define WS_STATUS_CODE_VIOLATES_POLICY 1008
/*
 *  1009 indicates that an endpoint is terminating the connection
 *  because it has received a message that is too big for it to
 *  process.
 */
#define WS_STATUS_CODE_TOO_BIG 1009
/*
 *  1010 indicates that an endpoint (client) is terminating the
 *  connection because it has expected the server to negotiate one or
 *  more extension, but the server didn't return them in the response
 *  message of the WebSocket handshake.  The list of extensions that
 *  are needed SHOULD appear in the /reason/ part of the Close frame.
 *  Note that this status code is not used by the server, because it
 *  can fail the WebSocket handshake instead.
 */
#define WS_STATUS_CODE_EXTENSION_ERROR 1010
/*
 *  1011 indicates that a server is terminating the connection because
 *  it encountered an unexpected condition that prevented it from
 *  fulfilling the request.
 */
#define WS_STATUS_CODE_UNEXPECTED_CONDITION 1011
/*
 *  1015 is a reserved value and MUST NOT be set as a status code in a
 *  Close control frame by an endpoint.  It is designated for use in
 *  applications expecting a status code to indicate that the
 *  connection was closed due to a failure to perform a TLS handshake
 *  (e.g., the server certificate can't be verified).
 */
#define WS_STATUS_CODE_RESERVED3 1015

namespace gsdm {

class WSProtocol {
public:
  
  WSProtocol();
  virtual ~WSProtocol();

  virtual bool HandlerHandshake();
  virtual bool HandlerText(const std::string &data) = 0;
  virtual bool HandlerBinary(const uint8_t *data, uint32_t len) = 0;

  bool SendWhole(const void *data, uint32_t len, bool binary);
  bool SendFragmentHead(const void *data, uint32_t len, bool binary);
  bool SendFragment(const void *data, uint32_t len, bool binary);
  bool SendFragmentTail(const void *data, uint32_t len, bool binary);

protected:
  std::string GetRequestHeader(const std::string &key);
  std::string GetRequestHeaderArgs(const std::string &key);
  void SetRespondHeader(const std::string &key, const std::string &value);
  /*
   * return -1 is error, else return used length
   */
  int HandlerProtocol(uint8_t *data, uint32_t len);
  void SendClose();
  virtual bool SendWSData(const void *data, uint32_t len) = 0;
  
private:

  bool ParseHeader(char *header);
  bool CheckHeader();
  void Respond101();
  void Respond400();
  void SendPing(const std::string &message);
  void SendPong(const std::string &message);
  struct WsFrame;
  bool SendWS(const WsFrame *frame, const void *data, uint32_t len);

  struct WsFrame {
    uint8_t frame_fin;
    uint8_t frame_rsv1;
    uint8_t frame_rsv2;
    uint8_t frame_rsv3;
    uint8_t frame_opcode;
    uint8_t frame_masked;
    uint8_t frame_masking_key[4];
    uint64_t frame_payload_length;
    uint64_t frame_masked_extension_length;
  };

  typedef std::unordered_map<std::string, std::string> WSHeaderHash;
  WSHeaderHash ws_request_header_hash_;
  WSHeaderHash ws_respond_header_hash_;
  WSHeaderHash ws_header_args_hash_;
  int ws_state_;
  IOBuffer ws_fragmentation_buffer_;
  WsFrame ws_request_frame_;
  uint8_t ws_frame_opcode_continue_;
  bool ws_is_send_close_;
  uint16_t ws_status_code_;
};

}

#endif 

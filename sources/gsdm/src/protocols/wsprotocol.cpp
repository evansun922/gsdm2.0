#include "protocols/wsprotocol.h"
#include "protocols/urlcode.h"
#include <utils/crypto.h>
#include "logging/logger.h"

namespace gsdm {

WSProtocol::WSProtocol()
  : ws_state_(WS_STATE_HANDSHAKE),ws_frame_opcode_continue_(0),
    ws_is_send_close_(false),ws_status_code_(WS_STATUS_CODE_GOING_AWAY) {  
  ws_fragmentation_buffer_.Initialize(4096);
  memset(&ws_request_frame_, 0, sizeof(WsFrame));
}

WSProtocol::~WSProtocol() {
  
}

bool WSProtocol::HandlerHandshake() {
  if ( !GetRequestHeader(WS_HEADER_WS_PROTOCOL).empty() ) {
    FATAL("Server is not support %s in request-hander.", WS_HEADER_WS_PROTOCOL);
    return false;
  }

  // if ( !GetRequestHeader(WS_HEADER_WS_EXTENSIONS).empty() ) {
  //   DEBUG("Server is not support %s: %s in request-hander.", WS_HEADER_WS_EXTENSIONS, STR(GetRequestHeader(WS_HEADER_WS_EXTENSIONS)));
  // }

  // TODO should check WS_HEADER_ORIGIN in header
  // std::string origin = GetRequestHeader(WS_HEADER_ORIGIN);

  return true;
}

bool WSProtocol::SendWhole(const void *data, uint32_t len, bool binary) {
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  frame.frame_fin = 1;
  frame.frame_opcode = binary ? 0x2 : 0x1;
  return SendWS(&frame, data, len);
}

bool WSProtocol::SendFragmentHead(const void *data, uint32_t len, bool binary) {
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  // frame.frame_fin = 0;
  frame.frame_opcode = binary ? 0x2 : 0x1;
  return SendWS(&frame, data, len);
}

bool WSProtocol::SendFragment(const void *data, uint32_t len, bool binary) {
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  // frame.frame_fin = 0;
  // frame.frame_opcode = binary ? 0x2 : 0x1;
  return SendWS(&frame, data, len);
}

bool WSProtocol::SendFragmentTail(const void *data, uint32_t len, bool binary) {
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  frame.frame_fin = 1;
  // frame.frame_opcode = binary ? 0x2 : 0x1;
  return SendWS(&frame, data, len);
}

std::string WSProtocol::GetRequestHeader(const std::string &key) {
  return ws_request_header_hash_[key];
}

std::string WSProtocol::GetRequestHeaderArgs(const std::string &key) {
  return ws_header_args_hash_[key];
}

void WSProtocol::SetRespondHeader(const std::string &key, const std::string &value) {
  ws_respond_header_hash_[key] = value;
}

int WSProtocol::HandlerProtocol(uint8_t *data, uint32_t len) {  
  uint8_t *buffer = data;
  uint32_t size = len;

  if ( WS_STATE_OPEN == ws_state_ ) {
    while ( true ) {
      if ( 0 == ws_request_frame_.frame_payload_length ) {
        if ( 2 > size )
          return buffer - data;
      
        uint32_t parse_pos = 2;
        ws_request_frame_.frame_fin = buffer[0] & 0x80 ? 1 : 0;
        ws_request_frame_.frame_rsv1 = buffer[0] & 0x40 ? 1 : 0;
        ws_request_frame_.frame_rsv2 = buffer[0] & 0x20 ? 1 : 0;
        ws_request_frame_.frame_rsv3 = buffer[0] & 0x10 ? 1 : 0;            
        ws_request_frame_.frame_opcode = buffer[0] & 0xf;

        ws_request_frame_.frame_masked = buffer[1] & 0x80 ? 1 : 0;
        ws_request_frame_.frame_payload_length = buffer[1] & 0x7f;
        if ( 0x7e == ws_request_frame_.frame_payload_length ) {
          if ( 4 > size ) {
            ws_request_frame_.frame_payload_length = 0;
            return buffer - data;
          }
          ws_request_frame_.frame_payload_length = (buffer[2] << 8 | buffer[3]);
          parse_pos = 4;
        } else if ( 0x7f == ws_request_frame_.frame_payload_length ) {
          if ( 10 > size ) {
            ws_request_frame_.frame_payload_length = 0;
            return buffer - data;
          }
          uint8_t *ci, *co;
          ci = buffer+2;
          co = (uint8_t *)&ws_request_frame_.frame_payload_length;
          co[0] = ci[7];
          co[1] = ci[6];
          co[2] = ci[5];
          co[3] = ci[4];
          co[4] = ci[3];
          co[5] = ci[2];
          co[6] = ci[1];
          co[7] = ci[0];
          parse_pos = 10;
        }
      
        if ( ws_request_frame_.frame_masked ) {
          if ( 4 > size - parse_pos ) {
            ws_request_frame_.frame_payload_length = 0;
            return buffer - data;
          }
          memcpy(ws_request_frame_.frame_masking_key, buffer + parse_pos, 4);
          parse_pos += 4;
        }

        if ( 0 == ws_request_frame_.frame_fin &&
             ( 1 <= ws_request_frame_.frame_opcode && 7 >= ws_request_frame_.frame_opcode ) && 
             0 == ws_frame_opcode_continue_ ) {
          ws_frame_opcode_continue_ = ws_request_frame_.frame_opcode;
        } else if ( 1 == ws_request_frame_.frame_fin &&
                    ( 0 == ws_request_frame_.frame_opcode ) &&
                    ( 1 <= ws_frame_opcode_continue_ && 7 >= ws_frame_opcode_continue_ ) ) {
          ws_request_frame_.frame_opcode = ws_frame_opcode_continue_;
          ws_frame_opcode_continue_ = 0;
        }

        buffer += parse_pos;
        size -= parse_pos;
      }

      if ( size < ws_request_frame_.frame_payload_length )
        return buffer - data;

      if ( ws_request_frame_.frame_masked_extension_length ) {
        // TODO extension
      }

      if ( ws_request_frame_.frame_masked ) {
        for (uint64_t i = 0; i < ws_request_frame_.frame_payload_length; i++) {
          buffer[i] = buffer[i] ^ ws_request_frame_.frame_masking_key[i % 4];
        }
      }

      if ( ws_request_frame_.frame_fin ) {
        uint8_t *b = GETIBPOINTER(ws_fragmentation_buffer_);
        uint32_t s = GETAVAILABLEBYTESCOUNT(ws_fragmentation_buffer_);
        if ( s ) {
          ws_fragmentation_buffer_.ReadFromBuffer(buffer, (uint32_t)ws_request_frame_.frame_payload_length);
          b = GETIBPOINTER(ws_fragmentation_buffer_);
          s = GETAVAILABLEBYTESCOUNT(ws_fragmentation_buffer_);
        } else {
          b = buffer;
          s = (uint32_t)ws_request_frame_.frame_payload_length;
        }

        if ( 0x1 == ws_request_frame_.frame_opcode ) {
          // text frame
          std::string msg((char *)b, s);
          if ( !HandlerText(msg) ) {
            ws_status_code_ = WS_STATUS_CODE_GOING_AWAY;
            SendClose();
            return -1;
          }
        } else if ( 0x2 == ws_request_frame_.frame_opcode ) {
          // binary frame
          if ( !HandlerBinary(b, s) ) {
            ws_status_code_ = WS_STATUS_CODE_GOING_AWAY;
            SendClose();
            return -1;
          }
        } else if ( 0x8 == ws_request_frame_.frame_opcode ) {
          // close
          if ( 2 == s ) {
            uint16_t status_code = b[0] << 8 | b[1];
            if ( WS_STATUS_CODE_GOING_AWAY != status_code && WS_STATUS_CODE_NORMAL_CLOSE != status_code ) {
              WARN("Websocket client is closing, but status code %u is unnormal.", status_code);
            }
            ws_status_code_ = WS_STATUS_CODE_NORMAL_CLOSE;
            SendClose();
            return -1;
          }
        } else if ( 0x9 == ws_request_frame_.frame_opcode ) {
          // ping
          std::string message((char *)b, s);
          SendPong(message);
        } else if ( 0xa == ws_request_frame_.frame_opcode ) {
          // pong 
        
        } else {
          WARN("unkown frame_opcode %u, so close", ws_request_frame_.frame_opcode);
          ws_status_code_ = WS_STATUS_CODE_PROTOCOL_ERROR;
          SendClose();
          return -1;
        }
        ws_fragmentation_buffer_.IgnoreAll();
      } else {
        ws_fragmentation_buffer_.ReadFromBuffer(buffer, (uint32_t)ws_request_frame_.frame_payload_length);
      }

      buffer += ws_request_frame_.frame_payload_length;
      size -= ws_request_frame_.frame_payload_length;
      memset(&ws_request_frame_, 0, sizeof(WsFrame));
    }
  } else if ( WS_STATE_HANDSHAKE == ws_state_ ) {

    char *find = rstrstr((const char *)buffer, (int)size, "\r\n\r\n", 4);
    if (find == NULL) {
      return 0;
    }

    *find = 0;

    if ( !ParseHeader((char *)buffer) ) {      
      return -1;
    }

    if ( !CheckHeader() ) {
      Respond400();
      return -1;
    }
    
    // for caller
    if ( !HandlerHandshake() ) {
      Respond400();
      return -1;
    }

    Respond101();
    ws_state_ = WS_STATE_OPEN;
    return ((uint8_t*)find - buffer) + 4;
  }

  return -1;
}

bool WSProtocol::ParseHeader(char *header) {
  // GET /chat HTTP/1.1
  char *tmp2, *tmp1 = strstr(header, "\r\n");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;
  tmp2 = tmp1 + 2;

  // app
  header += sizeof("GET");
  tmp1 = strstr(header, " ");
  if ( NULL == tmp1 )
    return false;
  *tmp1 = 0;

  // char decode[8192] = { 0 };
  // decodeURIComponent(header, decode, sizeof(decode));
  // header = decode;

  char *find = strstr(header, "?");
  if ( find ) {
    *find = 0;
    find++;
    char *arg1, *arg2;
    while ( NULL != (arg1 = strstr(find, "&")) ) {
      *arg1 = 0;
      arg2 = strstr(find, "=");
      if ( NULL == arg2 ) {
        ws_header_args_hash_[find] = "";
      } else {
        *arg2 = 0;
        ++arg2;
        ws_header_args_hash_[find] = arg2;
      }
      find = ++arg1;
    }

    arg2 = strstr(find, "=");
    if ( NULL == arg2 ) {
      ws_header_args_hash_[find] = "";
    } else {
      *arg2 = 0;
      ++arg2;
      ws_header_args_hash_[find] = arg2;
    }
  } 
  ws_request_header_hash_[WS_HEADER_URI] = header;

  // http version
  header = ++tmp1;
  ws_request_header_hash_[WS_HEADER_VERSION] = header;

  // Host: 10.16.14.183:9999
  header = tmp2;
  while ( NULL != (tmp1 = strstr(header, "\r\n")) ) {
    *tmp1 = 0;
    tmp2 = tmp1 + 2;
    tmp1 = strstr(header, ":");
    if ( NULL == tmp1 )
      return false;
    *tmp1 = 0;
    ws_request_header_hash_[header] = tmp1 + 2;
    header = tmp2;
  }

  tmp1 = strstr(header, ":");
  if ( NULL == tmp1 )
    return true;
  *tmp1 = 0;
  ws_request_header_hash_[header] = tmp1 + 2;

  return true;
}

bool WSProtocol::CheckHeader() {
  WSHeaderHash::iterator iter = ws_request_header_hash_.find(WS_HEADER_VERSION);
  if ( ws_request_header_hash_.end() == iter || "HTTP/1.1" != MAP_VAL(iter) ) {
    FATAL("The request is not HTTP/1.1");
    return false;
  }
  
  iter = ws_request_header_hash_.find(WS_HEADER_UPGRADE);
  if (ws_request_header_hash_.end() == iter || NULL == strcasestr(STR(MAP_VAL(iter)), "websocket") ) {
    FATAL("There is no \"Upgrade: websocket\" in header");
    return false;
  }

  iter = ws_request_header_hash_.find(WS_HEADER_CONNECTION);
  if (ws_request_header_hash_.end() == iter || NULL == strcasestr(STR(MAP_VAL(iter)), "upgrade") ) {
    FATAL("There is no \"Connection: upgrade\" in header");
    return false;
  }

  if (ws_request_header_hash_.end() == ws_request_header_hash_.find(WS_HEADER_WS_KEY) ) {
    FATAL("There is no Sec-WebSocket-Key in header");
    return false;
  }

  // if (ws_request_header_hash_.end() == ws_request_header_hash_.find(WS_HEADER_ORIGIN) )
  //   return false;  

  iter = ws_request_header_hash_.find(WS_HEADER_WS_VERSION);
  if (ws_request_header_hash_.end() == iter || "13" != MAP_VAL(iter) ) {
    FATAL("The Sec-WebSocket-Version must be 13.");
    return false;
  }

  return true;
}

void WSProtocol::Respond101() {
  std::string accept_key = ws_request_header_hash_[WS_HEADER_WS_KEY] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  uint8_t key[SHA_DIGEST_LENGTH];
  gsdm::sha1(STR(accept_key), accept_key.length(), key);
  accept_key = gsdm::b64(key, SHA_DIGEST_LENGTH);

  std::stringstream respond;
  respond << "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n";
  respond << "Sec-WebSocket-Accept: " << accept_key << "\r\n";
  FOR_UNORDERED_MAP(ws_respond_header_hash_,std::string,std::string,i) {
    respond << MAP_KEY(i) << ": " << MAP_VAL(i) << "\r\n";
  }
  respond << "\r\n";

  std::string r = respond.str();
  SendWSData(STR(r), r.length());
}

void WSProtocol::Respond400() {
  std::string accept_key = ws_request_header_hash_[WS_HEADER_WS_KEY] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  uint8_t key[SHA_DIGEST_LENGTH];
  gsdm::sha1(STR(accept_key), accept_key.length(), key);
  accept_key = gsdm::b64(key, SHA_DIGEST_LENGTH);

  // TODO
  std::string respond = format("HTTP/1.1 400 Bad Request\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n          \
                               Sec-WebSocket-Accept: %s\r\nSec-WebSocket-Version: 13\r\n\r\n", STR(accept_key));
  SendWSData(STR(respond), respond.length());
}

void WSProtocol::SendClose() {
  if ( ws_is_send_close_ )
    return;

  ws_is_send_close_ = true;
  uint8_t v[2];
  v[1] = ws_status_code_ & 0xff;
  v[0] = ws_status_code_ >> 8;
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  frame.frame_fin = 1;
  frame.frame_opcode = 0x8;
  SendWS(&frame, v, 2);
}

void WSProtocol::SendPing(const std::string &message) {
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  frame.frame_fin = 1;
  frame.frame_opcode = 0x9;
  SendWS(&frame, STR(message), message.length());
}

void WSProtocol::SendPong(const std::string &message) {
  WsFrame frame;
  memset(&frame, 0, sizeof(WsFrame));
  frame.frame_fin = 1;
  frame.frame_opcode = 0xa;
  SendWS(&frame, STR(message), message.length());
}

bool WSProtocol::SendWS(const WsFrame *frame, const void *data, uint32_t len) {
  uint8_t buffer[16];
  memset(buffer, 0, 16);
  uint32_t send_pos = 2;

  if ( frame->frame_fin ) {
    buffer[0] |= 0x80;
  }
  if ( frame->frame_rsv1 ) {
    buffer[0] |= 0x40;
  }
  if ( frame->frame_rsv2 ) {
    buffer[0] |= 0x20;
  }
  if ( frame->frame_rsv3 ) {
    buffer[0] |= 0x10;
  }
  buffer[0] |= frame->frame_opcode;

  if ( frame->frame_masked ) {
    buffer[1] |= 0x80;
  }
  /*
    %x00-7D
    %x7E frame-payload-length-16
    %x7F frame-payload-length-63
  */
  if ( 0x7d >= len ) {
    buffer[1] |= ((uint8_t)len);
  } else if ( 0xffff >= len ) {
    buffer[1] |= 0x7e;
    uint16_t l = (uint16_t)len;
    buffer[send_pos+1] = l & 0xff;
    buffer[send_pos] = l >> 8;
    send_pos += 2;
  } else {
    buffer[1] |= 0x7f;
    uint64_t l = len;
    uint8_t *ci, *co;
    ci = (uint8_t *)&l;
    co = buffer + send_pos;
    co[0] = ci[7];
    co[1] = ci[6];
    co[2] = ci[5];
    co[3] = ci[4];
    co[4] = ci[3];
    co[5] = ci[2];
    co[6] = ci[1];
    co[7] = ci[0];

    send_pos += 8;
  }

  if ( frame->frame_masked ) {
    memcpy(buffer + send_pos, frame->frame_masking_key, 4);
    send_pos += 4;

    uint8_t *b = new uint8_t[len];
    memcpy(b, data, len);
    for (uint32_t i = 0; i < len; i++) {
      b[i] = b[i] ^ frame->frame_masking_key[i % 4];
    }

    if ( !SendWSData(buffer, send_pos) ) {
      delete [] b;
      return false;
    }
    
    bool rs = SendWSData(b, len);
    delete [] b;
    return rs;
  }

  if ( !SendWSData(buffer, send_pos) )
    return false;

  return SendWSData(data, len);
}


}


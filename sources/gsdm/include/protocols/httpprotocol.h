/*
 *  Copyright (c) 2016,
 *  It complete http protocol on server 
 *  sunlei (sswin0922@163.com)
 */

#ifndef _HTTPPROTOCOL_H_
#define _HTTPPROTOCOL_H_

#include "platform/linuxplatform.h"
#include "buffering/iobuffer.h"


// http header key
#define HTTP_HEADER_URI           "uri"
#define HTTP_HEADER_ARGS          "args"
#define HTTP_HEADER_VERSION       "http_version"
#define HTTP_HEADER_HOST          "Host"
#define HTTP_HEADER_PORT          "Port"
#define HTTP_HEADER_CONNECTION    "Connection"
#define HTTP_HEADER_ORIGIN        "Origin"
#define HTTP_CONTENT_LENGTH       "Content-Length"

#define HTTP_RESPOND_CODE_200     "200 OK"
#define HTTP_RESPOND_CODE_204     "204 No Content"
#define HTTP_RESPOND_CODE_302     "302 Moved Temporarily"
#define HTTP_RESPOND_CODE_400     "400 Bad Request"
#define HTTP_RESPOND_CODE_404     "404 Not Found"

#define HTTP_CONTENT_TYPE_TEXT    "text/html"
#define HTTP_CONTENT_TYPE_TS      "video/MP2T"   //ts
#define HTTP_CONTENT_TYPE_M3U8    "application/vnd.apple.mpegurl"
#define HTTP_CONTENT_TYPE_FLV     "video/x-flv"
#define HTTP_CONTENT_TYPE_MP4     "video/mp4"

namespace gsdm {

class HTTPProtocol {
public:
  
  HTTPProtocol();
  virtual ~HTTPProtocol();

  // server mode
  virtual bool Handler();
  // if len == 0 is chunked mode
  bool SendHead(const std::string &code,
                const std::string &content_type);
  bool SendContent(const void *data, uint32_t len);
  bool SendChunked(const void *data, uint32_t len);

  // client mode
  virtual bool HandlerRespond(const char *data, int len);
  bool ParseUrl(const std::string &url);
  bool HTTPGet();

protected:
  std::string GetRequestArgs(const std::string &key);
  std::string GetRequestHeader(const std::string &key);

  void SetRequestHeader(const std::string &key,
                        const std::string &value);

  void SetRespondHeader(const std::string &key,
                        const std::string &value);
  void SetRespondHeaderForContentLength(uint64_t length);
  void SetRespondHeaderForTransferEncoding();
  void SetRespondHeaderForConnectionKeepalive();
  void SetRespondHeaderForConnectionClose();

  int GetHttpCode();
  virtual bool SendHTTPData(const void *data, uint32_t len) = 0;
  /*
   * return -1 is error, else return used length
   */
  int HandlerProtocol(uint8_t *data, uint32_t len);
  int HandlerProtocolRespond(uint8_t *data, uint32_t len);

private:
  bool ParseHeader(char *header);
  bool ParseRespondHeader(char *header);
  bool CheckHeader();

  typedef std::unordered_map<std::string, std::string> HTTPHeaderHash;
  HTTPHeaderHash http_request_header_hash_;
  HTTPHeaderHash http_respond_header_hash_;
  HTTPHeaderHash http_args_hash_;
  int content_length_; // default -1, 0 is chunked
  int code_;
};

}

#endif 
